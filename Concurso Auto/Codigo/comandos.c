#include <avr/io.h>
#include <stdint.h>
#include "comandos.h"
#include "bluetooth.h"
#include "motores.h"

// Variables para recibir la velocidad
static uint8_t recibiendo_velocidad = 0;
static uint8_t valor_velocidad = 0;
static uint8_t cantidad_digitos = 0;

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
       return;
    }
	
	char dato= bluetooth_leer();
	
	//S siempre tiene prioridad para detener
	if(dato=='S')
	{
		  recibiendo_velocidad = 0;
		  valor_velocidad = 0;
		  cantidad_digitos = 0;

		  motores_detener();
		  bluetooth_enviar_texto("OK\r\n");
		  return;
	}
	
	  // Comienzo de un comando de velocidad
	  if (dato == 'V')
	  {
		  recibiendo_velocidad = 1;
		  valor_velocidad = 0;
		  cantidad_digitos = 0;
		  return;
	  }
	  
	   // Procesar los caracteres de velocidad
	   if (recibiendo_velocidad)
	   {
		   if (dato >= '0' && dato <= '9')
		   {
			   uint8_t digito = dato - '0';

			   if (cantidad_digitos >= 3 ||
			   valor_velocidad > (100 - digito) / 10)
			   {
				   recibiendo_velocidad = 0;
				   bluetooth_enviar_texto("ERROR\r\n");
				   return;
			   }

			   valor_velocidad =
			   valor_velocidad * 10 + digito;

			   cantidad_digitos++;
			   return;
		   }

		   // Fin del comando de velocidad
		   if (dato == ';')
		   {
			   if (cantidad_digitos > 0)
			   {
				   uint8_t pwm =
				   ((uint32_t)valor_velocidad * 255 + 50)
				   / 100;

				   motores_velocidad(pwm);
				   bluetooth_enviar_texto("OK\r\n");
			   }
			   else
			   {
				   bluetooth_enviar_texto("ERROR\r\n");
			   }

			   recibiendo_velocidad = 0;
			   return;
		   }

		   // Comando de velocidad incorrecto
		   recibiendo_velocidad = 0;
		   bluetooth_enviar_texto("ERROR\r\n");
		   return;
	   }

	   // Ignorar terminadores de linea
	   if (dato == '\r' || dato == '\n')
	   {
		   return;
	   }

	   // Comandos de movimiento existentes
	   comandos_ejecutar(dato);
   }
}



