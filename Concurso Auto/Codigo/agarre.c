
#define F_CPU 16000000UL

#include <avr/io.h>
#include <stdint.h>
#include "agarre.h"

// =====================================
// CONFIGURACION DEL SERVOMOTOR
// =====================================

// Servo conectado a D9 (PB1 / OC1A)
#define SERVO PB1

// Angulos iniciales de la pinza
#define ANGULO_ABIERTO  30
#define ANGULO_CERRADO  90

// Timer1: prescaler 8, 0.5 us por cuenta
// Periodo de 20 ms (50 Hz)
#define PERIODO_SERVO 39999

// Pulsos iniciales entre 1 y 2 ms
#define PULSO_MINIMO 2000
#define PULSO_MAXIMO 4000

// =====================================
// INICIALIZACION
// =====================================

void agarre_init(void)
{
	// D9 como salida
	DDRB |= (1 << SERVO);

	// Detener Timer1 durante configuracion
	TCCR1A = 0;
	TCCR1B = 0;

	// Periodo de 20 ms
	ICR1 = PERIODO_SERVO;

	// Posicion inicial: pinza cerrada
	agarre_angulo(ANGULO_CERRADO);

	// Fast PWM, modo 14
	// Salida no inversora en OC1A
	TCCR1A = (1 << COM1A1) | (1 << WGM11);

	// Prescaler 8
	TCCR1B = (1 << WGM13)
	| (1 << WGM12)
	| (1 << CS11);
}

// =====================================
// CONTROL DEL ANGULO
// =====================================

void agarre_angulo(uint8_t angulo)
{
	uint16_t pulso;

	// Limitar angulo entre 0 y 180
	if (angulo > 180)
	{
		angulo = 180;
	}

	// Convertir angulo a pulso PWM
	pulso = PULSO_MINIMO
	+ (uint16_t)(((uint32_t)angulo *
	(PULSO_MAXIMO - PULSO_MINIMO)) / 180UL);

	// Actualizar posicion del servo
	OCR1A = pulso;
}

// =====================================
// ABRIR PINZA
// =====================================

void agarre_abrir(void)
{
	agarre_angulo(ANGULO_ABIERTO);
}

// =====================================
// CERRAR PINZA
// =====================================

void agarre_cerrar(void)
{
	agarre_angulo(ANGULO_CERRADO);
}
