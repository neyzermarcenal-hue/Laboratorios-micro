#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>

// Nuestras librerias
#include "watchdog.h"

// ====================================
// PRUEBA 5: WATCHDOG TIMER
// ====================================

int main(void)
{
    // Configurar LED PB0 (D8) como salida
    DDRB |= (1 << PB0);

    // Apagar LED inicialmente
    PORTB &= ~(1 << PB0);

    // Habilitar interrupciones globales
    sei();

    while (1)
    {
        // ====================================
        // INICIAR WATCHDOG
        // ====================================

        // Apagar LED
        PORTB &= ~(1 << PB0);

        // Configurar Watchdog con tiempo de 1 segundo
        watchdog_iniciar(WDT_1S);

        // Inicializar contador de interrupciones
        uint8_t contador = 0;

        // ====================================
        // CONTAR 5 INTERRUPCIONES
        // ====================================

        while (contador < 5)
        {
            // Comprobar si ocurrio la interrupcion
            if (watchdog_evento())
            {
                // Encender LED
                PORTB |= (1 << PB0);

                // Mantener encendido 100 ms
                _delay_ms(100);

                // Apagar LED
                PORTB &= ~(1 << PB0);

                // Incrementar contador
                contador++;
            }
        }

        // ====================================
        // DETENER WATCHDOG
        // ====================================

        // Desactivar Watchdog
        watchdog_detener();

        // Esperar 2 segundos antes de repetir
        _delay_ms(2000);
    }
}
