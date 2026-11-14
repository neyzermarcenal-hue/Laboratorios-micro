
#define F_CPU 16000000UL

#include <avr/io.h>

// Nuestra libreria
#include "motores.h"
#include "bluetooth.h"
#include "comandos.h"

int main(void)
{
    // Inicializar motores y PWM
    motores_init();

    // Inicializar comunicacion Bluetooth
    bluetooth_init();

    // Confirmar inicio del sistema
    bluetooth_enviar_texto("ROBOT LISTO\r\n");

    while (1)
    {
        // Procesar comandos Bluetooth
        comandos_procesar();
    }

    return 0;
}
