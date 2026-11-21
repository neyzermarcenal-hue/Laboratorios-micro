
#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <stdint.h>
#include "agarre.h"

// Configuracion del servo
#define ANGULO_ABIERTO 30U
#define ANGULO_CERRADO 90U

#define PASO_GRADOS 2U

#define PULSO_MINIMO 2000U
#define PULSO_MAXIMO 4000U
#define PERIODO_SERVO 39999U

static uint8_t angulo_actual = ANGULO_CERRADO;
static uint8_t angulo_objetivo = ANGULO_CERRADO;

// Actualizar ancho de pulso del servo
static void servo_escribir(uint8_t angulo)
{
	OCR1A = (uint16_t)(
	PULSO_MINIMO +
	((uint32_t)angulo *
	(PULSO_MAXIMO - PULSO_MINIMO)) / 180UL
	);
}

// Inicializar Timer1 para servo en D9
void agarre_init(void)
{
	DDRB |= (1 << PB1);

	TCCR1A = 0;
	TCCR1B = 0;

	ICR1 = PERIODO_SERVO;

	servo_escribir(angulo_actual);

	TCCR1A = (1 << COM1A1) |
	(1 << WGM11);

	TCCR1B = (1 << WGM13) |
	(1 << WGM12) |
	(1 << CS11);
}

// Orden de apertura
void agarre_abrir(void)
{
	angulo_objetivo = ANGULO_ABIERTO;
}

// Orden de cierre
void agarre_cerrar(void)
{
	angulo_objetivo = ANGULO_CERRADO;
}

// Actualizar gradualmente el angulo
void agarre_tarea(void)
{
	if (angulo_actual < angulo_objetivo)
	{
		uint8_t restante =
		(uint8_t)(angulo_objetivo - angulo_actual);

		angulo_actual +=
		(restante >= PASO_GRADOS) ?
		PASO_GRADOS : restante;
	}
	else if (angulo_actual > angulo_objetivo)
	{
		uint8_t restante =
		(uint8_t)(angulo_actual - angulo_objetivo);

		angulo_actual -=
		(restante >= PASO_GRADOS) ?
		PASO_GRADOS : restante;
	}

	servo_escribir(angulo_actual);
}
