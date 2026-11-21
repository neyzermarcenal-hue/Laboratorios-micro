#include <avr/io.h>
#include <stdint.h>
#include "comandos.h"
#include "bluetooth.h"
#include "motores.h"
#include "agarre.h"
#include "robot.h"

// Trama: 0xAA, CMD, VAL, CMD XOR VAL

// Variables para recibir la velocidad
static uint8_t recibiendo_velocidad = 0;
static uint8_t valor_velocidad = 0;
static uint8_t velocidad_valida = 0;

//Velocidad

void comandos_init(void)
{
	etapa = 0;
}

// Verificar que el comando y su valor sean validos
static uint8_t trama_semantica_valida(
uint8_t cmd,
uint8_t valor)
{
	if (cmd > 7U)
	return 0;

	if (cmd == 0U || cmd == 5U || cmd == 6U)
	return valor == 0U;

	return valor <= 100U;
}

// Procesar bytes recibidos desde Bluetooth
void comandos_procesar(uint32_t ahora)
{
	while (bluetooth_disponible())
	{
		uint8_t b = bluetooth_leer();

		switch (etapa)
		{
			case 0:
			// Buscar byte de inicio
			if (b == 0xAA)
			etapa = 1;
			break;

			case 1:
			// Recibir comando
			cmd_actual = b;
			etapa = 2;
			break;

			case 2:
			// Recibir valor
			valor_actual = b;
			etapa = 3;
			break;

			default:
			// Cuarto byte: checksum
			etapa = 0;

			if ((uint8_t)(cmd_actual ^ valor_actual) == b &&
			trama_semantica_valida(cmd_actual,
			valor_actual))
			{
				robot_recibir_trama(
				cmd_actual,
				valor_actual,
				ahora);
			}
			break;
		}
	}
}
