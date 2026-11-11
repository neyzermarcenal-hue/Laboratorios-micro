
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
// PRUEBA 3: SLEEP POWER-DOWN
// ====================================

int main(void)
{
    // Configurar LED PB0 (D8) como salida
    DDRB |= (1 << PB0);
    PORTB &= ~(1 << PB0);

    // Inicializar modulos
    interrupciones_init();
    mediciones_init();

    // Habilitar interrupciones globales
    sei();

    while (1)
    {
        // Apagar LED antes de dormir
        PORTB &= ~(1 << PB0);

        // Esperar que el boton este suelto
        while (!(PIND & (1 << PD2)))
        {
        }

        // ====================================
        // CONFIGURAR INTERRUPCION INT0
        // ====================================

        // Desactivar interrupciones globales
        cli();

        // Deshabilitar INT0 temporalmente
        EIMSK &= ~(1 << INT0);

        // Configurar INT0 por nivel bajo
        EICRA &= ~((1 << ISC01) | (1 << ISC00));

        // Limpiar interrupcion pendiente
        EIFR = (1 << INTF0);

        // ====================================
        // ENTRAR EN POWER-DOWN
        // ====================================

        // Seleccionar modo Power-Down
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);

        // Preparar entrada a Sleep
        sleep_enable();

        // Habilitar INT0
        EIMSK |= (1 << INT0);

        // Habilitar interrupciones globales
        sei();

        // Dormir hasta presionar el boton
        sleep_cpu();

        // ====================================
        // DESPERTAR DEL MICROCONTROLADOR
        // ====================================

        // Desactivar Sleep
        sleep_disable();

        // Deshabilitar INT0 para evitar
        // interrupciones repetidas
        EIMSK &= ~(1 << INT0);

        // Iniciar señal de medicion
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
