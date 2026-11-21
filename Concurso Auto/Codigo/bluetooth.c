
#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include "bluetooth.h"

// =====================================
// CONFIGURACION UART
// =====================================

#define BAUD 9600UL
#define BUFFER_RX 64U

static volatile uint8_t buffer_rx[BUFFER_RX];
static volatile uint8_t rx_cabeza = 0;
static volatile uint8_t rx_cola = 0;

// Inicializar comunicacion UART
void bluetooth_init(void)
{
    uint16_t ubrr =
    (uint16_t)(F_CPU / (16UL * BAUD) - 1UL);

    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)ubrr;

    UCSR0A = 0;

    // 8 bits de datos, sin paridad, 1 bit de parada
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);

    // Habilitar recepcion, transmision e interrupcion RX
    UCSR0B = (1 << RXEN0) |
    (1 << TXEN0) |
    (1 << RXCIE0);
}

// Interrupcion cuando llega un dato Bluetooth
ISR(USART_RX_vect)
{
	uint8_t dato = UDR0;

	uint8_t siguiente =
	(uint8_t)((rx_cabeza + 1U) & (BUFFER_RX - 1U));

	if (siguiente != rx_cola)
	{
		buffer_rx[rx_cabeza] = dato;
		rx_cabeza = siguiente;
	}
}

// Comprobar si existen datos recibidos
uint8_t bluetooth_disponible(void)
{
     return rx_cabeza != rx_cola;
}

// Leer un byte del buffer
uint8_t bluetooth_leer(void)
{
	uint8_t dato = buffer_rx[rx_cola];

	rx_cola =
	(uint8_t)((rx_cola + 1U) & (BUFFER_RX - 1U));

	return dato;
}

// Enviar un byte por UART
void bluetooth_enviar(uint8_t dato)
{
	while (!(UCSR0A & (1 << UDRE0)))
	{
	}

	UDR0 = dato;
}
