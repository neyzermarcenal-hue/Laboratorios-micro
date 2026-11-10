#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/atomic.h>

#include "watchdog.h"

// Bandera de interrupcion del WDT
static volatile uint8_t evento_wdt = 0;

// Interrupcion del Watchdog
ISR(WDT_vect)
{
    evento_wdt = 1;
}

// Configurar WDT en modo interrupcion
void watchdog_iniciar(TiempoWDT tiempo)
{
    uint8_t configuracion = 0;

    if (tiempo == WDT_16MS)
    {
        configuracion = 0;
    }
    else if (tiempo == WDT_250MS)
    {
        configuracion = (1 << WDP2);
    }
    else if (tiempo == WDT_1S)
    {
        configuracion = (1 << WDP2) | (1 << WDP1);
    }
    else if (tiempo == WDT_8S)
    {
        configuracion = (1 << WDP3) | (1 << WDP0);
    }

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        // Desactivar WDT anterior
        wdt_reset();

        // Habilitar modificacion del registro
        WDTCSR = (1 << WDCE) | (1 << WDE);

        // Configurar solo interrupcion
        WDTCSR = (1 << WDIE) | configuracion;

        evento_wdt = 0;
    }
}

// Consultar y limpiar el evento
uint8_t watchdog_evento(void)
{
    uint8_t evento;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        evento = evento_wdt;
        evento_wdt = 0;
    }

    return evento;
}

// Desactivar Watchdog
void watchdog_detener(void)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        wdt_reset();
        WDTCSR = (1 << WDCE) | (1 << WDE);
        WDTCSR = 0;
        evento_wdt = 0;
    }
}

