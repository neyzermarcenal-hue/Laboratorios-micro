#include <avr/io.h>
#include <avr/interrupt.h>

#include "reloj.h"

// Frecuencia inicial del Arduino Uno
static uint32_t frecuencia_actual = 16000000UL;

void reloj_configurar(FrecuenciaReloj frecuencia)
{
    uint8_t prescaler = 0;
    uint32_t nueva_frecuencia = 16000000UL;

    if (frecuencia == RELOJ_8MHZ)
    {
        prescaler = 1;
        nueva_frecuencia = 8000000UL;
    }
    else if (frecuencia == RELOJ_2MHZ)
    {
        prescaler = 3;
        nueva_frecuencia = 2000000UL;
    }
    else if (frecuencia == RELOJ_250KHZ)
    {
        prescaler = 6;
        nueva_frecuencia = 250000UL;
    }
    else if (frecuencia == RELOJ_125KHZ)
    {
        prescaler = 7;
        nueva_frecuencia = 125000UL;
    }

    // Guardar estado de las interrupciones
    uint8_t estado_interrupciones = SREG;

    // Desactivar interrupciones temporalmente
    cli();

    // Habilitar modificacion de CLKPR
    CLKPR = (1 << CLKPCE);

    // Aplicar el nuevo prescaler
    CLKPR = prescaler;

    // Restaurar estado de interrupciones
    SREG = estado_interrupciones;

    frecuencia_actual = nueva_frecuencia;
}

uint32_t reloj_obtener_hz(void)
{
    return frecuencia_actual;
}

