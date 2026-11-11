#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <stdint.h>

// Nuestras librerias
#include "interrupciones.h"
#include "mediciones.h"

// ====================================
// PRUEBA 1: SLEEP IDLE
// ====================================

int main(void)
{
    // Configurar LED PB0 (D8) como salida
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

        // Entrar en Sleep Idle
        sei();
        sleep_cpu();

        // Desactivar Sleep al despertar
        sleep_disable();

        // Indicar que el Arduino desperto
        mediciones_inicio();

        // Encender LED
        PORTB |= (1 << PB0);

        // Mantener LED encendido 500 ms
        _delay_ms(500);

        // Apagar LED
        PORTB &= ~(1 << PB0);

        // Finalizar señal de medicion
        mediciones_fin();

        // Esperar que suelte el boton
        while (!(PIND & (1 << PD2)))
        {
        }

        // Limpiar evento anterior
        boton_presionado();
    }
}
