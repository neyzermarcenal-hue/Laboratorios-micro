
#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#include "motores.h"
#include "agarre.h"
#include "bluetooth.h"
#include "comandos.h"
#include "sensores.h"
#include "robot.h"
#include "reloj.h"

int main(void)
{
	// Guardar causa del ultimo reinicio
	uint8_t reset_causa = MCUSR;
	MCUSR = 0;

	// Inicializacion de modulos
	motores_init();
	agarre_init();
	bluetooth_init();
	sensores_init();
	comandos_init();

	robot_init((reset_causa & (1 << BORF)) != 0);

	reloj_init();

	// Habilitar interrupciones globales
	sei();

	// Variables para scheduler
	uint32_t t10 = 0;
	uint32_t t20 = 0;
	uint32_t t200 = 0;

	while (1)
	{
		uint32_t ahora = reloj_ms();

		// Tareas cada 10 ms
		if ((uint32_t)(ahora - t10) >= 10UL)
		{
			t10 += 10UL;

			comandos_procesar(ahora);
			robot_tarea(ahora);
		}

		// Tarea de pinza cada 20 ms
		if ((uint32_t)(ahora - t20) >= 20UL)
		{
			t20 += 20UL;

			agarre_tarea();
		}

		// Telemetria cada 200 ms
		if ((uint32_t)(ahora - t200) >= 200UL)
		{
			t200 += 200UL;

			sensores_tarea();
			robot_telemetria();
		}
	}
}
