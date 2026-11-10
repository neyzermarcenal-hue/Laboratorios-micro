#include <avr/io.h>

#include "mediciones.h"

// Pin de prueba: PB1 (Arduino D9)
#define PIN_MEDICION PB1

// Configurar el pin como salida
void mediciones_init(void)
{
    DDRB |= (1 << PIN_MEDICION);
    PORTB &= ~(1 << PIN_MEDICION);
}

// Colocar el pin en HIGH
void mediciones_inicio(void)
{
    PORTB |= (1 << PIN_MEDICION);
}

// Colocar el pin en LOW
void mediciones_fin(void)
{
    PORTB &= ~(1 << PIN_MEDICION);
}

