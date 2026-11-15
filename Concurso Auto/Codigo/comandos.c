#include <avr/io.h>
#include <stdint.h>
#include "comandos.h"
#include "bluetooth.h"
#include "motores.h"

// Variables para recibir la velocidad
static uint8_t recibiendo_velocidad = 0;
static uint8_t valor_velocidad = 0;
static uint8_t velocidad_valida = 0;

//Velocidad

static void aplicar_velocidad(uint8_t porcentaje)
{
	uint8_t pwm;

	// Limitar el porcentaje a 100
	if (porcentaje > 100)
	{
		porcentaje = 100;
	}

	// Convertir porcentaje a PWM
	pwm = (uint8_t)(((uint32_t)porcentaje * 255UL) / 100UL);

	motores_velocidad(pwm);
}


// =====================================
// INTERPRETACION DE COMANDOS
// =====================================

void comandos_ejecutar(char comando)
{
	// Procesamiento de velocidad: V70;
	if (recibiendo_velocidad)
	{
		if (comando >= '0' && comando <= '9')
		{
			velocidad_valida = 1;

			valor_velocidad = valor_velocidad * 10
			+ (comando - '0');

			if (valor_velocidad > 100)
			{
				valor_velocidad = 100;
			}

			return;
		}

		if (comando == ';')
		{
			if (velocidad_valida)
			{
				aplicar_velocidad(
				(uint8_t)valor_velocidad
				);
			}

			recibiendo_velocidad = 0;
			valor_velocidad = 0;
			velocidad_valida = 0;
			return;
		}

		// Cancelar un comando de velocidad incompleto
		recibiendo_velocidad = 0;
		valor_velocidad = 0;
		velocidad_valida = 0;
	}

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
			
		case 'L':
		   motores_izquierda();
		   break;
		   
		case 'R':
		   motores_derecha();
		   break;

        case 'S':
            // Detener
            motores_detener();
            break;
			
       case 'V':
       recibiendo_velocidad = 1;
       valor_velocidad = 0;
       velocidad_valida = 0;
       break;

       case 'O':
       // Pendiente: abrir pinza
       break;

       case 'C':
       // Pendiente: cerrar pinza
       break;

       default:
       // Ignorar comandos desconocidos
       break;
       }
}

// =====================================
// PROCESAR COMUNICACION BLUETOOTH
// =====================================

void comandos_procesar(void)
{
	while (bluetooth_disponible())
	{
		char comando = bluetooth_leer();

		comandos_ejecutar(comando);
	}
}