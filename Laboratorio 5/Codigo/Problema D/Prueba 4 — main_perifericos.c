
#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>

// Nuestras librerias
#include "interrupciones.h"
#include "perifericos.h"

// ====================================
// PRUEBA 4: DESACTIVACION DE PERIFERICOS
// ====================================

int main(void)
{
    // Configurar LED PB0 (D8) como salida
    DDRB |= (1 << PB0);
    PORTB &= ~(1 << PB0);

    // Inicializar interrupciones para el boton D2
    interrupciones_init();

    // Inicializar perifericos
    perifericos_init();

    // Habilitar interrupciones globales
    sei();

    while (1)
    {
        // Restaurar todos los perifericos
        perifericos_restaurar();

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
        // DESACTIVAR PERIFERICOS
        // ====================================

        // Desactivar modulos que no utilizamos
        periferico_desactivar(
            (1 << PRADC)  |
            (1 << PRSPI)  |
            (1 << PRTWI)  |
            (1 << PRTIM1) |
            (1 << PRTIM2)
        );

        // ====================================
        // INDICAR DESACTIVACION
        // ====================================

        // Encender LED
        PORTB |= (1 << PB0);

        // Mantener LED encendido 1 segundo
        _delay_ms(1000);

        // Apagar LED
        PORTB &= ~(1 << PB0);

        // ====================================
        // RESTAURAR PERIFERICOS
        // ====================================

        // Volver a habilitar todos los modulos
        perifericos_restaurar();

        // Esperar 1 segundo antes de repetir
        _delay_ms(1000);
    }
}
