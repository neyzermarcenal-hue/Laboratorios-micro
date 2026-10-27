/*
 * A.c
 *
 * Created: 27/10/2026 0:27:12
 * Author : cvarg
 */ 
#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define BAUD      9600
#define MY_UBRR   (F_CPU/16/BAUD-1)

/*UART*/

void uart_init(uint16_t ubrr) {
	UBRR0H = (uint8_t)(ubrr >> 8);
	UBRR0L = (uint8_t)ubrr;
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_print(const char *s) {
	while (*s) {
		while (!(UCSR0A & (1 << UDRE0)));
		UDR0 = *s++;
	}
}

/*MEMORIA EEPROM*/

#define EE_MARCA_DIR   0x00
#define EE_MARCA_VAL   0xB7
#define EE_BASE        0x10
#define TAM_CUENTA     8
#define N_CUENTAS      3
#define OFS_UID        0
#define OFS_PIN        4
#define OFS_SALDO      6

const uint8_t uid_fabrica[N_CUENTAS][4] = {
	{0x73, 0xF5, 0xD0, 0x26},
	{0x93, 0x96, 0x07, 0x2D},
	{0x8D, 0x6B, 0x2D, 0x03}
};

const uint16_t pin_fabrica[N_CUENTAS]   = {1234, 4321, 1111};
const uint16_t saldo_fabrica[N_CUENTAS] = {1000, 2500, 500};

void EEPROM_write(uint16_t direccion, uint8_t dato) {
	while (EECR & (1 << EEPE));
	EEAR = direccion;
	EEDR = dato;
	cli();
	EECR = (1 << EEMPE);
	EECR = (1 << EEMPE) | (1 << EEPE);
	sei();
}

uint8_t EEPROM_read(uint16_t direccion) {
	while (EECR & (1 << EEPE));
	EEAR = direccion;
	EECR |= (1 << EERE);
	return EEDR;
}

void EEPROM_update(uint16_t direccion, uint8_t dato) {
	if (EEPROM_read(direccion) != dato)
	EEPROM_write(direccion, dato);
}

void EEPROM_update16(uint16_t direccion, uint16_t valor) {
	EEPROM_update(direccion, (uint8_t)(valor & 0xFF));
	EEPROM_update(direccion + 1, (uint8_t)(valor >> 8));
}

uint16_t dir_cuenta(uint8_t i) {
	return EE_BASE + (uint16_t)i * TAM_CUENTA;
}

void eeprom_precargar(void) {
	uint8_t i, j;
	for (i = 0; i < N_CUENTAS; i++) {
		for (j = 0; j < 4; j++)
		EEPROM_write(dir_cuenta(i) + OFS_UID + j, uid_fabrica[i][j]);
		EEPROM_update16(dir_cuenta(i) + OFS_PIN, pin_fabrica[i]);
		EEPROM_update16(dir_cuenta(i) + OFS_SALDO, saldo_fabrica[i]);
	}
	EEPROM_write(EE_MARCA_DIR, EE_MARCA_VAL);
}
/*LCD*/

#define LCD_ADDR 0x27
#define LCD_RS   0x01
#define LCD_EN   0x04
#define LCD_BL   0x08

void TWI_init(void) {
	TWBR = 0x48;
	TWSR = 0x00;
	TWCR = (1 << TWEN);
}

void TWI_start(void) {
	TWCR = (1 << TWSTA) | (1 << TWEN) | (1 << TWINT);
	while (!(TWCR & (1 << TWINT)));
}

void TWI_stop(void) {
	TWCR = (1 << TWSTO) | (1 << TWEN) | (1 << TWINT);
}

void TWI_write(uint8_t dato) {
	TWDR = dato;
	TWCR = (1 << TWEN) | (1 << TWINT);
	while (!(TWCR & (1 << TWINT)));
}

void pcf_escribir(uint8_t dato) {
	TWI_start();
	TWI_write(LCD_ADDR << 1);
	TWI_write(dato | LCD_BL);
	TWI_stop();
}

void lcd_nibble(uint8_t nibble, uint8_t rs) {
	uint8_t d = (nibble & 0xF0) | rs;
	pcf_escribir(d | LCD_EN);
	_delay_us(2);
	pcf_escribir(d);
	_delay_us(50);
}

void lcd_enviar(uint8_t valor, uint8_t rs) {
	lcd_nibble(valor & 0xF0, rs);
	lcd_nibble((uint8_t)(valor << 4), rs);
}

void lcd_comando(uint8_t c) {
	lcd_enviar(c, 0);
	if (c <= 0x03) _delay_ms(2);
}

void lcd_dato(char c) {
	lcd_enviar((uint8_t)c, LCD_RS);
}

void lcd_init(void) {
	_delay_ms(50);
	lcd_nibble(0x30, 0); _delay_ms(5);
	lcd_nibble(0x30, 0); _delay_us(150);
	lcd_nibble(0x30, 0); _delay_us(150);
	lcd_nibble(0x20, 0);
	lcd_comando(0x28);
	lcd_comando(0x0C);
	lcd_comando(0x06);
	lcd_comando(0x01);
}

void lcd_linea(uint8_t fila, char *texto) {
	uint8_t n = 0;
	lcd_comando((fila == 0) ? 0x80 : 0xC0);
	while (texto[n] != '\0' && n < 16) {
		lcd_dato(texto[n]);
		n++;
	}
	while (n < 16) {
		lcd_dato(' ');
		n++;
	}
}

#define LED_VERDE PD6
#define LED_ROJO  PD7
#define BUZZER    PB0

char keypad[4][4] = {
	{'A', 'B', 'C', 'D'},
	{'3', '6', '9', '#'},
	{'2', '5', '8', '*'},
	{'1', '4', '7', '0'}
};

void pines_init(void) {
	DDRD |= (1 << LED_VERDE) | (1 << LED_ROJO);
	DDRB |= (1 << BUZZER);
	DDRC |= 0x0F; PORTC |= 0x0F;
	DDRD &= ~0x3C; PORTD |= 0x3C;
}
/*TECLADO*/

char teclado_leer(void) {
	uint8_t fila, col;
	for (fila = 0; fila < 4; fila++) {
		PORTC |= 0x0F;
		PORTC &= ~(1 << fila);
		_delay_us(10);
		for (col = 0; col < 4; col++) {
			if (!(PIND & (1 << (col + 2)))) {
				PORTC |= 0x0F;
				return keypad[fila][col];
			}
		}
	}
	PORTC |= 0x0F;
	return 0;
}

int main(void)
{
	pines_init();
	uart_init(MY_UBRR);
	TWI_init();
	lcd_init();

	if (EEPROM_read(EE_MARCA_DIR) != EE_MARCA_VAL) {
		eeprom_precargar();
	}

	lcd_linea(0, "ATM Iniciado");
	lcd_linea(1, "Press Key...");
	uart_print("\r\n[ATM] Drivers base iniciados.");

    while (1) 
    {
		char k = teclado_leer();
		if (k != 0) {
			uart_print("\r\nTecla: ");
			UDR0 = k;
			_delay_ms(200);
		}
    }
}

