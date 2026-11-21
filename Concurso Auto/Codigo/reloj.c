#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <stdint.h>
#include "reloj.h"

static volatile uint32_t milisegundos = 0;

void reloj_init(void)
{
    TCCR2A = (1 << WGM21);
    TCCR2B = 0;

    OCR2A = 249;
    TCNT2 = 0;

    TIMSK2 |= (1 << OCIE2A);

    TCCR2B = (1 << CS22);
}

ISR(TIMER2_COMPA_vect)
{
    milisegundos++;
}

uint32_t reloj_ms(void)
{
    uint32_t copia;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        copia = milisegundos;
    }

    return copia;
}
