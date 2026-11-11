#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

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

    // Habilitar interrupciones globales
    sei();

    while (1)
    {
        if (PRUEBA_ACTUAL == PRUEBA_IDLE)
        {
            // Indicar que el sistema esta activo
            PORTB |= (1 << PB0);

            // Entrar en modo Idle
            entrar_idle();

            // Ejecutar al despertar
            PORTB &= ~(1 << PB0);
            mediciones_inicio();

            // Esperar una nueva pulsacion
            while (!boton_presionado())
            {
            }

            mediciones_fin();
        }
    }
}

