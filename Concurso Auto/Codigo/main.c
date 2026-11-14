
#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>

// Nuestra libreria
#include "motores.h"
#include "bluetooth.h"

int main(void)
{
    // Inicializar los motores y el PWM
    motores_init();
    // Inicializar comunicacion Bluetooth
    bluetooth_init();
    
    // Confirmar que el sistema inicio
    bluetooth_enviar_texto("ROBOT LISTO\r\n");

    while (1)
    {
        // Verificar si llego un dato por Bluetooth
        if (bluetooth_disponible())
        {
            char comando = bluetooth_leer();
        
            // Confirmar el caracter recibido
            bluetooth_enviar_texto("RECIBIDO: ");
            bluetooth_enviar(comando);
            bluetooth_enviar_texto("\r\n");
        }
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

