
#define F_CPU 16000000UL

#include <avr/io.h>

// Nuestras librerias
#include "motores.h"
#include "bluetooth.h"
#include "comandos.h"
#include "agarre.h"

int main(void)
{
	// Inicializar motores
	motores_init();

	// Inicializar pinza
	agarre_init();

	// Inicializar Bluetooth
	bluetooth_init();

	// Mensaje de inicio
	bluetooth_enviar_texto("ROBOT LISTO\r\n");

	while (1)
	{
		// Procesar comandos recibidos
		comandos_procesar();
	}

	return 0;
}
