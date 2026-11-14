
#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <stdint.h>
#include "bluetooth.h"

// =====================================
// CONFIGURACION UART
// =====================================

#define BAUD_RATE 9600UL
#define UBRR_VALOR ((F_CPU / (16UL * BAUD_RATE)) - 1UL)

// =====================================
// INICIALIZACION BLUETOOTH
// =====================================

void bluetooth_init(void)
{
    // Configurar velocidad de comunicacion
    UBRR0H = (uint8_t)(UBRR_VALOR >> 8);
    UBRR0L = (uint8_t)UBRR_VALOR;

    // Modo UART normal
    UCSR0A = 0;

    // Habilitar receptor y transmisor
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);

    // 8 bits de datos, sin paridad, 1 stop
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

// =====================================
// VERIFICAR DATOS DISPONIBLES
// =====================================

uint8_t bluetooth_disponible(void)
{
    return (UCSR0A & (1 << RXC0)) != 0;
}

// =====================================
// LEER CARACTER RECIBIDO
// =====================================

char bluetooth_leer(void)
{
    // Esperar hasta recibir un dato
    while (!bluetooth_disponible());

    return UDR0;
}

// =====================================
// ENVIAR CARACTER
// =====================================

void bluetooth_enviar(char dato)
{
    // Esperar hasta poder transmitir
    while (!(UCSR0A & (1 << UDRE0)));

    UDR0 = dato;
}

// =====================================
// ENVIAR TEXTO
// =====================================

void bluetooth_enviar_texto(const char *texto)
{
    while (*texto != '\0')
    {
        bluetooth_enviar(*texto);
        texto++;
    }
}

