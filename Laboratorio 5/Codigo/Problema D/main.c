#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <util/delay.h>
#include <avr/sleep.h>

// Nuestras librerias
#include "sleep.h"
#include "interrupciones.h"
#include "perifericos.h"
#include "watchdog.h"
#include "reloj.h"
#include "mediciones.h"

// Seleccion de experimentos
#define PRUEBA_IDLE 1
#define PRUEBA_ADC_NOISE 2
#define PRUEBA_POWER_DOWN 3
#define PRUEBA_PERIFERICOS 4
#define PRUEBA_WDT 5
#define PRUEBA_RELOJ 6

// Elegimos la prueba
#define PRUEBA_ACTUAL PRUEBA_IDLE

int main(void)
{
    // Configurar LED PB0 como salida
    DDRB |= (1 << PB0);
    PORTB &= ~(1 << PB0);

    // Inicializar modulos
    interrupciones_init();
    mediciones_init();

    // Configurar modo Sleep Idle
    set_sleep_mode(SLEEP_MODE_IDLE);

    // Habilitar interrupciones globales
    sei();

   
    while (1)
    {
        if (PRUEBA_ACTUAL == PRUEBA_IDLE)
        {
            // Apagar LED antes de dormir
            PORTB &= ~(1 << PB0);

            // Esperar a que el boton este suelto
            while (!(PIND & (1 << PD2)))
            {
            }

            // Limpiar evento anterior
            boton_presionado();

            // Desactivar interrupciones globales
            cli();

            // Deshabilitar INT0 temporalmente
            EIMSK &= ~(1 << INT0);

            // Limpiar interrupcion pendiente
            EIFR = (1 << INTF0);

            // Preparar entrada a Sleep
            sleep_enable();
    
            // Habilitar INT0
            EIMSK |= (1 << INT0);

            sei();
            sleep_cpu();
            sleep_disable();

            // El Arduino desperto
            mediciones_inicio();
            PORTB |= (1 << PB0);

            // Mantener LED encendido 500 ms
            _delay_ms(500);

            // Apagar LED y señal de medicion
            PORTB &= ~(1 << PB0);
            mediciones_fin();

            // Esperar que suelte el boton
            while (!(PIND & (1 << PD2)))
            {
            }

            // Limpiar evento anterior
            boton_presionado();
        }
    }
}

