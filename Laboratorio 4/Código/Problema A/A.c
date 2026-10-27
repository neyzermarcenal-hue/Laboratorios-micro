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

int main(void)
{
    /* Replace with your application code */
    while (1) 
    {
    }
}

