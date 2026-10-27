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

/* Cambiar a 1 para simulación en Proteus, 0 para la placa física */
#define SIMULACION 0

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

void uart_init(uint16_t ubrr) { UBRR0H = (uint8_t)(ubrr >> 8); UBRR0L = (uint8_t)ubrr; UCSR0B = (1 << RXEN0) | (1 << TXEN0); UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); }
void uart_print(const char *s) { while (*s) { while (!(UCSR0A & (1 << UDRE0))); UDR0 = *s++; } }

/*MEMORIA EEPROM*/

#define EE_MARCA_DIR   0x00
#define EE_MARCA_VAL   0xB7
#define EE_BASE        0x10
#define TAM_CUENTA     8
#define N_CUENTAS      3
#define OFS_UID        0
#define OFS_PIN        4
#define OFS_SALDO      6

const uint8_t uid_fabrica[N_CUENTAS][4] = {{0x73, 0xF5, 0xD0, 0x26}, {0x93, 0x96, 0x07, 0x2D}, {0x8D, 0x6B, 0x2D, 0x03}};
void EEPROM_write(uint16_t dir, uint8_t dato) { while (EECR & (1 << EEPE)); EEAR = dir; EEDR = dato; cli(); EECR = (1 << EEMPE); EECR = (1 << EEMPE) | (1 << EEPE); sei(); }
uint8_t EEPROM_read(uint16_t dir) { while (EECR & (1 << EEPE)); EEAR = dir; EECR |= (1 << EERE); return EEDR; }

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

#define SS_PIN   PB2
#define RST_PIN  PB1

void spi_init(void)
{
	DDRB |= (1 << PB3) | (1 << PB5) | (1 << PB2) | (1 << PB1);
	PORTB |= (1 << SS_PIN) | (1 << RST_PIN);
	DDRB &= ~(1 << PB4);
	SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0);
}

uint8_t spi_transfer(uint8_t data)
{
	SPDR = data;
	while (!(SPSR & (1 << SPIF)));
	return SPDR;
}

void rc522_write_reg(uint8_t reg, uint8_t val)
{
	PORTB &= ~(1 << SS_PIN);
	spi_transfer((reg << 1) & 0x7E);
	spi_transfer(val);
	PORTB |= (1 << SS_PIN);
}

uint8_t rc522_read_reg(uint8_t reg)
{
	uint8_t val;
	PORTB &= ~(1 << SS_PIN);
	spi_transfer(((reg << 1) & 0x7E) | 0x80);
	val = spi_transfer(0x00);
	PORTB |= (1 << SS_PIN);
	return val;
}

void rc522_set_bit(uint8_t reg, uint8_t mask)
{
	rc522_write_reg(reg, rc522_read_reg(reg) | mask);
}

void rc522_clear_bit(uint8_t reg, uint8_t mask)
{
	rc522_write_reg(reg, rc522_read_reg(reg) & (~mask));
}

void mfrc522_init(void)
{
	PORTB &= ~(1 << RST_PIN);
	_delay_ms(10);
	PORTB |= (1 << RST_PIN);
	_delay_ms(50);

	rc522_write_reg(0x01, 0x0F);
	_delay_ms(50);

	rc522_write_reg(0x2A, 0x8D);
	rc522_write_reg(0x2B, 0x3E);
	rc522_write_reg(0x2D, 30);
	rc522_write_reg(0x2C, 0);
	rc522_write_reg(0x15, 0x40);
	rc522_write_reg(0x11, 0x3D);
	rc522_write_reg(0x26, 0x70);

	uint8_t temp = rc522_read_reg(0x14);
	if (!(temp & 0x03)) {
		rc522_set_bit(0x14, 0x03);
	}
}

uint8_t rc522_to_card(uint8_t cmd, uint8_t *send_data, uint8_t send_len, uint8_t *back_data, uint8_t *back_len)
{
	uint8_t status = 0;
	uint8_t irqEn = 0x00, waitIRq = 0x00, lastBits, n;
	uint16_t i;

	if (cmd == 0x0C) {
		irqEn = 0x77;
		waitIRq = 0x30;
	}

	rc522_write_reg(0x02, irqEn | 0x80);
	rc522_clear_bit(0x04, 0x80);
	rc522_set_bit(0x0A, 0x80);
	rc522_write_reg(0x01, 0x00);

	for (i = 0; i < send_len; i++) {
		rc522_write_reg(0x09, send_data[i]);
	}

	rc522_write_reg(0x01, cmd);
	if (cmd == 0x0C) {
		rc522_set_bit(0x0D, 0x80);
	}

	i = 2000;
	do {
		n = rc522_read_reg(0x04);
		i--;
	} while ((i != 0) && !(n & 0x01) && !(n & waitIRq));

	rc522_clear_bit(0x0D, 0x80);

	if (i != 0) {
		if (!(rc522_read_reg(0x06) & 0x1B)) {
			status = 1;
			if (cmd == 0x0C) {
				n = rc522_read_reg(0x0A);
				lastBits = rc522_read_reg(0x0C) & 0x07;
				if (lastBits) {
					*back_len = (n - 1) * 8 + lastBits;
					} else {
					*back_len = n * 8;
				}
				if (n == 0) {
					n = 1;
				}
				if (n > 16) {
					n = 16;
				}
				for (i = 0; i < n; i++) {
					back_data[i] = rc522_read_reg(0x09);
				}
			}
		}
	}
	return status;
}

uint8_t rc522_request(uint8_t req_mode, uint8_t *tag_type)
{
	uint8_t status, backBits;
	rc522_write_reg(0x0D, 0x07);
	tag_type[0] = req_mode;
	status = rc522_to_card(0x0C, tag_type, 1, tag_type, &backBits);
	if ((status != 1) || (backBits != 0x10)) {
		status = 0;
	}
	return status;
}

uint8_t rc522_anticoll(uint8_t *serNum)
{
	uint8_t status, i, serNumCheck = 0, unLen;
	rc522_write_reg(0x0D, 0x00);
	serNum[0] = 0x93;
	serNum[1] = 0x20;
	status = rc522_to_card(0x0C, serNum, 2, serNum, &unLen);
	if (status == 1) {
		for (i = 0; i < 4; i++) {
			serNumCheck ^= serNum[i];
		}
		if (serNumCheck != serNum[4]) {
			status = 0;
		}
	}
	return status;
}

#if SIMULACION
#define LCD_ADDR     0x20
#else
#define LCD_ADDR     0x27
#endif

#define MAX_INTENTOS   3
#define TIMEOUT        1500

#define ST_ESPERA_TARJETA   0
#define ST_PEDIR_PIN        1
#define ST_MENU             2
#define ST_CONSULTA         3
#define ST_RETIRO           4
#define ST_DEPOSITO         5
#define ST_BLOQUEO          6

#define IN_CANCEL   0
#define IN_OK       1
#define IN_TIMEOUT  2

uint8_t rfid_leer_uid(uint8_t *uid)
{
	uint8_t status, str[16];

	status = rc522_request(0x26, str);
	if (status == 1) {
		status = rc522_anticoll(str);
		if (status == 1) {
			for (uint8_t i = 0; i < 4; i++) {
				uid[i] = str[i];
			}
			return 1;
		}
	}
	return 0;
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
/* ================= FUNCIONES AUXILIARES Y EEPROM 16 BITS ================= */

void uart_vaciar_rx(void)
{
	uint8_t basura;
	while (UCSR0A & (1 << RXC0)) {
		basura = UDR0;
	}
	(void)basura;
}

uint16_t EEPROM_read16(uint16_t direccion)
{
	uint16_t bajo = EEPROM_read(direccion);
	uint16_t alto = EEPROM_read(direccion + 1);
	return (alto << 8) | bajo;
}

uint8_t buscar_cuenta(uint8_t *uid)
{
	uint8_t i, j, iguales;
	for (i = 0; i < N_CUENTAS; i++) {
		iguales = 1;
		for (j = 0; j < 4; j++) {
			if (EEPROM_read(dir_cuenta(i) + OFS_UID + j) != uid[j]) {
				iguales = 0;
			}
		}
		if (iguales) {
			return i;
		}
	}
	return NO_ENCONTRADA;
}

/* ============================ UTILIDADES TEXTO ============================ */

void num_a_texto(uint16_t valor, char *texto)
{
	char tmp[6];
	uint8_t n = 0, i;

	if (valor == 0) {
		texto[0] = '0';
		texto[1] = '\0';
		return;
	}
	while (valor > 0) {
		tmp[n++] = '0' + (valor % 10);
		valor = valor / 10;
	}
	for (i = 0; i < n; i++) {
		texto[i] = tmp[n - 1 - i];
	}
	texto[n] = '\0';
}

uint16_t texto_a_num(char *texto)
{
	uint32_t valor = 0;
	uint8_t i = 0;

	while (texto[i] != '\0') {
		valor = valor * 10 + (texto[i] - '0');
		if (valor > 65535UL) {
			return 65535;
		}
		i++;
	}
	return (uint16_t)valor;
}

void uart_print_num(uint16_t valor)
{
	char t[6];
	num_a_texto(valor, t);
	uart_print(t);
}

void uart_print_uid(uint8_t *uid)
{
	const char *hex = "0123456789ABCDEF";
	char t[9];
	uint8_t i;

	for (i = 0; i < 4; i++) {
		t[2 * i]     = hex[uid[i] >> 4];
		t[2 * i + 1] = hex[uid[i] & 0x0F];
	}
	t[8] = '\0';
	uart_print(t);
}

/* ============================ CONTROL DE LEDS Y BUZZER ==================== */

void delay_ms(uint16_t n)
{
	while (n--) {
		_delay_ms(1);
	}
}

void led_verde(uint16_t ms)
{
	PORTD |= (1 << LED_VERDE);
	delay_ms(ms);
	PORTD &= ~(1 << LED_VERDE);
}

void led_rojo(uint16_t ms)
{
	PORTD |= (1 << LED_ROJO);
	delay_ms(ms);
	PORTD &= ~(1 << LED_ROJO);
}

void beep(uint16_t ms)
{
	uint16_t i;
	for (i = 0; i < ms; i++) {
		PORTB |= (1 << BUZZER);
		_delay_us(500);
		PORTB &= ~(1 << BUZZER);
		_delay_us(500);
	}
}

/* ============================ LECTURA DE TECLADO ========================== */

char teclado_tecla(void)
{
	char k = teclado_leer();
	if (k == 0) {
		return 0;
	}
	_delay_ms(20);
	if (teclado_leer() != k) {
		return 0;
	}
	while (teclado_leer() != 0);
	_delay_ms(20);
	return k;
}

char esperar_tecla(uint16_t ticks)
{
	char k;
	while (ticks > 0) {
		k = teclado_tecla();
		if (k != 0) {
			return k;
		}
		_delay_ms(10);
		ticks--;
	}
	return 0;
}

uint8_t leer_entrada(char *buf, uint8_t max, uint8_t min, uint8_t oculto)
{
	uint8_t largo = 0, i;
	char linea[17];
	char k;

	buf[0] = '\0';
	lcd_linea(1, "");

	while (1) {
		k = esperar_tecla(TIMEOUT);

		if (k == 0) {
			return IN_TIMEOUT;
		}

		if (k >= '0' && k <= '9') {
			if (largo < max) {
				buf[largo] = k;
				largo++;
				buf[largo] = '\0';
			}
			} else if (k == '*') {
			if (largo == 0) {
				return IN_CANCEL;
			}
			largo--;
			buf[largo] = '\0';
			} else if (k == '#') {
			if (largo >= min) {
				return IN_OK;
			}
		}

		for (i = 0; i < largo; i++) {
			if (oculto) {
				linea[i] = '*';
				} else {
				linea[i] = buf[i];
			}
		}
		linea[largo] = '\0';
		lcd_linea(1, linea);
	}
}

void esperar_retiro_tarjeta(void)
{
	#if !SIMULACION
	uint8_t uid[4];
	while (rfid_leer_uid(uid)) {
		_delay_ms(200);
	}
	#endif
}

void mostrar_saldo(uint8_t cuenta, char *titulo)
{
	char b[6];
	num_a_texto(EEPROM_read16(dir_cuenta(cuenta) + OFS_SALDO), b);
	lcd_linea(0, titulo);
	lcd_linea(1, b);
}

int main(void)
{
	uint8_t  estado = ST_ESPERA_TARJETA;
	uint8_t  nuevo = 1;
	uint8_t  cuenta = 0;
	uint8_t  intentos = 0;
	uint8_t  uid[4];
	uint8_t  r;
	uint16_t monto, saldo;
	char     k;
	char     buf[8];
	char     msg[] = "Intento X de 3";

	pines_init();
	
	/* Prueba inicial de LEDs */
	PORTD |= (1 << LED_VERDE) | (1 << LED_ROJO);
	_delay_ms(300);
	PORTD &= ~((1 << LED_VERDE) | (1 << LED_ROJO));

	uart_init(MY_UBRR);
	TWI_init();
	lcd_init();

	if (EEPROM_read(EE_MARCA_DIR) != EE_MARCA_VAL) {
		eeprom_precargar();
	}

	uart_print("\r\n==========================================");
	uart_print("\r\n[ATM] Cuentas cargadas en EEPROM:");
	for (uint8_t i = 0; i < N_CUENTAS; i++) {
		uint8_t temp_uid[4];
		for (uint8_t j = 0; j < 4; j++) {
			temp_uid[j] = EEPROM_read(dir_cuenta(i) + OFS_UID + j);
		}
		uart_print("\r\n Cuenta ");
		uart_print_num(i + 1);
		uart_print(": UID = ");
		uart_print_uid(temp_uid);
	}
	uart_print("\r\n==========================================");

	#if !SIMULACION
	spi_init();
	mfrc522_init();
	_delay_ms(100);
	#endif

	while (1) {

		switch (estado) {

			case ST_ESPERA_TARJETA:
			if (nuevo) {
				nuevo = 0;
				lcd_linea(0, "    CAJERO ATM");
				lcd_linea(1, "Acerque tarjeta");
				esperar_retiro_tarjeta();
				uart_vaciar_rx();
				uart_print("\r\n[ATM] Esperando tarjeta...");
			}
			if (rfid_leer_uid(uid)) {
				uart_print("\r\n[ATM] UID recibido: ");
				uart_print_uid(uid);
				cuenta = buscar_cuenta(uid);
				if (cuenta == NO_ENCONTRADA) {
					uart_print(" -> TARJETA NO REGISTRADA");
					lcd_linea(0, "Tarjeta no");
					lcd_linea(1, "registrada");
					
					beep(500);
					led_rojo(2000);
					
					nuevo = 1;
					} else {
					uart_print(" -> RECONOCIDA (Cuenta ");
					uart_print_num(cuenta + 1);
					uart_print(")");

					lcd_linea(0, "    TARJETA     ");
					lcd_linea(1, "   ACEPTADA     ");
					led_verde(1500);

					intentos = 0;
					estado = ST_PEDIR_PIN;
					nuevo = 1;
				}
			}
			_delay_ms(20);
			break;

			case ST_PEDIR_PIN:
			if (nuevo) {
				nuevo = 0;
				lcd_linea(0, "Ingrese PIN:");
			}
			r = leer_entrada(buf, 4, 4, 1);
			if (r == IN_OK) {
				if (texto_a_num(buf) == EEPROM_read16(dir_cuenta(cuenta) + OFS_PIN)) {
					uart_print("\r\n[ATM] PIN correcto");
					lcd_linea(0, "PIN correcto");
					lcd_linea(1, "");
					led_verde(800);
					estado = ST_MENU;
					} else {
					intentos++;
					uart_print("\r\n[ATM] PIN incorrecto, intento ");
					uart_print_num(intentos);
					if (intentos >= MAX_INTENTOS) {
						estado = ST_BLOQUEO;
						} else {
						msg[8] = '0' + intentos;
						lcd_linea(0, "PIN incorrecto");
						lcd_linea(1, msg);
						led_rojo(1500);
					}
				}
				} else {
				if (r == IN_TIMEOUT) {
					lcd_linea(0, "Tiempo agotado");
					} else {
					lcd_linea(0, "Operacion");
				}
				lcd_linea(1, "cancelada");
				led_rojo(1000);
				estado = ST_ESPERA_TARJETA;
			}
			nuevo = 1;
			break;

			case ST_MENU:
			if (nuevo) {
				nuevo = 0;
				lcd_linea(0, "A:Saldo B:Retiro");
				lcd_linea(1, "C:Deposito D:Sal");
			}
			k = esperar_tecla(TIMEOUT);
			if (k == 'A') {
				estado = ST_CONSULTA;
				nuevo = 1;
				} else if (k == 'B') {
				estado = ST_RETIRO;
				nuevo = 1;
				} else if (k == 'C') {
				estado = ST_DEPOSITO;
				nuevo = 1;
				} else if (k == 'D') {
				lcd_linea(0, "Gracias por usar");
				lcd_linea(1, "el cajero");
				delay_ms(1500);
				estado = ST_ESPERA_TARJETA;
				nuevo = 1;
				} else if (k == 0) {
				lcd_linea(0, "Tiempo agotado");
				lcd_linea(1, "cancelada");
				delay_ms(1500);
				estado = ST_ESPERA_TARJETA;
				nuevo = 1;
			}
			break;

			case ST_CONSULTA:
			mostrar_saldo(cuenta, "Saldo actual:");
			uart_print("\r\n[ATM] Consulta de saldo: ");
			uart_print_num(EEPROM_read16(dir_cuenta(cuenta) + OFS_SALDO));
			led_verde(500);
			esperar_tecla(500);
			estado = ST_MENU;
			nuevo = 1;
			break;

			case ST_RETIRO:
			lcd_linea(0, "Monto a retirar:");
			r = leer_entrada(buf, 5, 1, 0);
			if (r == IN_TIMEOUT) {
				lcd_linea(0, "Tiempo agotado");
				lcd_linea(1, "cancelada");
				delay_ms(1000);
				estado = ST_ESPERA_TARJETA;
				nuevo = 1;
				break;
			}
			if (r == IN_OK) {
				monto = texto_a_num(buf);
				saldo = EEPROM_read16(dir_cuenta(cuenta) + OFS_SALDO);
				
				if (monto == 0 || (monto % 500 != 0)) {
					uart_print("\r\n[ATM] Retiro rechazado");
					lcd_linea(0, "Monto no valido");
					lcd_linea(1, "Solo mult de 500");
					led_rojo(1500);
					} else if (monto > saldo) {
					uart_print("\r\n[ATM] Retiro rechazado: saldo insuficiente");
					lcd_linea(0, "Saldo");
					lcd_linea(1, "insuficiente");
					led_rojo(1500);
					} else {
					saldo = saldo - monto;
					EEPROM_update16(dir_cuenta(cuenta) + OFS_SALDO, saldo);
					uart_print("\r\n[ATM] Retiro: ");
					uart_print_num(monto);
					uart_print(" | Saldo: ");
					uart_print_num(saldo);
					lcd_linea(0, "Retiro exitoso");
					lcd_linea(1, "");
					
					beep(300);
					led_verde(1000);
					
					mostrar_saldo(cuenta, "Nuevo saldo:");
					delay_ms(2000);
				}
			}
			estado = ST_MENU;
			nuevo = 1;
			break;

			case ST_DEPOSITO:
			lcd_linea(0, "Monto deposito:");
			r = leer_entrada(buf, 5, 1, 0);
			if (r == IN_TIMEOUT) {
				lcd_linea(0, "Tiempo agotado");
				lcd_linea(1, "cancelada");
				delay_ms(1000);
				estado = ST_ESPERA_TARJETA;
				nuevo = 1;
				break;
			}
			if (r == IN_OK) {
				monto = texto_a_num(buf);
				saldo = EEPROM_read16(dir_cuenta(cuenta) + OFS_SALDO);
				
				if (monto == 0 || (monto % 500 != 0)) {
					uart_print("\r\n[ATM] Deposito rechazado: no es multiplo de 500");
					lcd_linea(0, "Monto no valido");
					lcd_linea(1, "Solo mult de 500");
					led_rojo(1500);
					} else if ((uint32_t)saldo + monto > 65535UL) {
					uart_print("\r\n[ATM] Deposito rechazado: limite de saldo");
					lcd_linea(0, "Limite de saldo");
					lcd_linea(1, "excedido");
					led_rojo(1500);
					} else {
					saldo = saldo + monto;
					EEPROM_update16(dir_cuenta(cuenta) + OFS_SALDO, saldo);
					uart_print("\r\n[ATM] Deposito: ");
					uart_print_num(monto);
					uart_print(" | Saldo: ");
					uart_print_num(saldo);
					lcd_linea(0, "Deposito exitoso");
					lcd_linea(1, "");
					led_verde(1000);
					mostrar_saldo(cuenta, "Nuevo saldo:");
					delay_ms(2000);
				}
			}
			estado = ST_MENU;
			nuevo = 1;
			break;

			case ST_BLOQUEO:
			uart_print("\r\n[ATM] 3 intentos fallidos: acceso bloqueado");
			lcd_linea(0, "Acceso bloqueado");
			lcd_linea(1, "Retire tarjeta");
			PORTD |= (1 << LED_ROJO);
			beep(400);
			delay_ms(200);
			beep(400);
			delay_ms(200);
			beep(400);
			delay_ms(1500);
			PORTD &= ~(1 << LED_ROJO);
			estado = ST_ESPERA_TARJETA;
			nuevo = 1;
			break;
		}
	}
}

