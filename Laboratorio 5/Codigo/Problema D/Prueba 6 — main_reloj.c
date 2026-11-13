#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>

// Nuestras librerias
#include "interrupciones.h"
#include "reloj.h"
#include "mediciones.h"

// ====================================
// PRUEBA 6: FRECUENCIA DEL RELOJ
// ====================================

int main(void)
{
    // ====================================
    // CONFIGURACION INICIAL
    // ====================================

    // Configurar LED PB0 (D8) como salida
    DDRB |= (1 << PB0);

    // Apagar LED inicialmente
    PORTB &= ~(1 << PB0);

    // Inicializar modulos
    interrupciones_init();
    mediciones_init();

    // Configurar frecuencia inicial de 16 MHz
    reloj_configurar(RELOJ_16MHZ);

    // Habilitar interrupciones globales
    sei();

    while (1)
    {
        // ====================================
        // RESTAURAR FRECUENCIA INICIAL
        // ====================================

        // Restaurar reloj a 16 MHz
        reloj_configurar(RELOJ_16MHZ);

        // Apagar LED
        PORTB &= ~(1 << PB0);

        // ====================================
        // ESPERAR PULSACION DEL BOTON
        // ====================================

        // Esperar a que se presione D2
        while (PIND & (1 << PD2))
        {
        }

        // Esperar a que se suelte el boton
        while (!(PIND & (1 << PD2)))
        {
        }

        // ====================================
        // MODIFICAR FRECUENCIA
        // ====================================

        // Cambiar frecuencia de 16 MHz a 2 MHz
        reloj_configurar(RELOJ_2MHZ);

        // ====================================
        // MEDIR TIEMPO DE EJECUCION
        // ====================================

        // Iniciar señal de medicion en D9
        mediciones_inicio();

        // Encender LED D8
        PORTB |= (1 << PB0);

        // Retardo calculado para F_CPU = 16 MHz
        // A 2 MHz dura aproximadamente 800 ms
        _delay_ms(100);

        // Apagar LED
        PORTB &= ~(1 << PB0);

        // Finalizar señal de medicion
        mediciones_fin();

        // ====================================
        // RESTAURAR RELOJ
        // ====================================

        // Volver a frecuencia de 16 MHz
        reloj_configurar(RELOJ_16MHZ);

        // Esperar 1 segundo antes de repetir
        _delay_ms(1000);
    }
}
