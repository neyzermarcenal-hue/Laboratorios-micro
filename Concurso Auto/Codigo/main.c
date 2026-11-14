
#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>

// Nuestra libreria
#include "motores.h"

int main(void)
{
    // Inicializar los motores y el PWM
    motores_init();

    while (1)
    {
        // AVANZAR
        motores_avanzar();
        _delay_ms(2000);

        // DETENER
        motores_detener();
        _delay_ms(1000);

        // RETROCEDER
        motores_retroceder();
        _delay_ms(2000);

        // DETENER
        motores_detener();
        _delay_ms(1000);
    }

    return 0;
}

