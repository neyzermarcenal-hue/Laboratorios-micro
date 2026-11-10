#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#include "interrupciones.h"

#define BOTON PD2

// Variable que registra el evento
static volatile uint8_t evento_boton = 0;

// Configuracion del boton INT0
void interrupciones_init(void)
{
    // PD2 como entrada
    DDRD &= ~(1 << BOTON);

    // Activar resistencia pull-up
    PORTD |= (1 << BOTON);

    // INT0 por flanco descendente
    EICRA |= (1 << ISC01);
    EICRA &= ~(1 << ISC00);

    // Limpiar interrupcion pendiente
    EIFR = (1 << INTF0);

    // Habilitar INT0
    EIMSK |= (1 << INT0);
}

// Interrupcion del boton
ISR(INT0_vect)
{
    evento_boton = 1;
}

// Consultar y limpiar el evento
uint8_t boton_presionado(void)
{
    uint8_t evento;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        evento = evento_boton;
        evento_boton = 0;
    }

    return evento;
}
