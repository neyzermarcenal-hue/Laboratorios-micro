
#include <avr/io.h>
#include "comandos.h"
#include "bluetooth.h"
#include "motores.h"

// =====================================
// INTERPRETACION DE COMANDOS
// =====================================

void comandos_ejecutar(char comando)
{
    switch (comando)
    {
        case 'F':
            // Avanzar
            motores_avanzar();
            break;

        case 'B':
            // Retroceder
            motores_retroceder();
            break;

        case 'S':
            // Detener
            motores_detener();
            break;

        default:
            // Comando no reconocido
            bluetooth_enviar_texto("ERROR\r\n");
            return;
    }

    bluetooth_enviar_texto("OK\r\n");
}

// =====================================
// PROCESAR COMUNICACION BLUETOOTH
// =====================================

void comandos_procesar(void)
{
    if (bluetooth_disponible())
    {
        char comando = bluetooth_leer();
        comandos_ejecutar(comando);
    }
}

