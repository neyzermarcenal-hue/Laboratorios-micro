
#include <avr/io.h>
#include "motores.h"

// Definicion de pines para el MX1508

// Motor A (izquierdo)
#define IN1 PD5   // D5 - PWM OC0B
#define IN2 PD4   // D4 - Direccion

// Motor B (derecho)
#define IN3 PD6   // D6 - PWM OC0A
#define IN4 PD7   // D7 - Direccion

#define PINES_MOTORES ((1 << IN1) | (1 << IN2) | \
(1 << IN3) | (1 << IN4))

// Control de velocidad 

// Velocidad entre 0 y 255
static uint8_t velocidad_actual = 0;

// Ultima configuracion aplicada
static int8_t ultima_dir_a = 0;
static int8_t ultima_dir_b = 0;
static uint8_t ultima_velocidad = 0;
static uint8_t configurado = 0;

// Direcciones:
//  1 = Adelante
// -1 = Atras
//  0 = Detenido

static void motores_aplicar(int8_t dir_a, int8_t dir_b)
{
	// Evitar reconfiguraciones innecesarias
	if (configurado &&
	dir_a == ultima_dir_a &&
	dir_b == ultima_dir_b &&
	velocidad_actual == ultima_velocidad)
	{
		return;
	}

	ultima_dir_a = dir_a;
	ultima_dir_b = dir_b;
	ultima_velocidad = velocidad_actual;
	configurado = 1;

	// Desconectar temporalmente salidas PWM
	// Mantener Timer0 en Fast PWM
	TCCR0A = (1 << WGM01) | (1 << WGM00);

	// Dejar todas las entradas del MX1508 en bajo
	PORTD &= ~PINES_MOTORES;
	
	OCR0B = velocidad_actual; // PWM motor izquierdo
	OCR0A = velocidad_actual; // PWM motor derecho

	// Velocidad cero: motores detenidos
	if (velocidad_actual == 0)
	{
		return;
	}

//MOTOR A

if (dir_a == 1)
	{
		// Adelante: IN1 PWM, IN2 bajo

		if (velocidad_actual == 255)
		{
			PORTD |= (1 << IN1);
		}
		else
		{
			// PWM no inversor en OC0B
			TCCR0A |= (1 << COM0B1);
		}
	}
	else if (dir_a == -1)
	{
		// Atras: IN1 PWM inverso, IN2 alto
		PORTD |= (1 << IN2);

		if (velocidad_actual < 255)
		{
			// PWM inversor en OC0B
			TCCR0A |= (1 << COM0B1) |
			(1 << COM0B0);
		}
	}

//MOTOR B

if (dir_b == 1)
	{
		// Adelante: IN3 PWM, IN4 bajo

		if (velocidad_actual == 255)
		{
			PORTD |= (1 << IN3);
		}
		else
		{
			// PWM no inversor en OC0A
			TCCR0A |= (1 << COM0A1);
		}
	}
	else if (dir_b == -1)
	{
		// Atras: IN3 PWM inverso, IN4 alto
		PORTD |= (1 << IN4);

		if (velocidad_actual < 255)
		{
			// PWM inversor en OC0A
			TCCR0A |= (1 << COM0A1) |
			(1 << COM0A0);
		}
	}
}


void motores_init(void)
{
   // Desactivar Timer0 durante configuracion
	TCCR0A = 0;
	TCCR0B = 0;

	// Entradas MX1508 inicialmente en bajo
	PORTD &= ~PINES_MOTORES;

	// Configurar los cuatro pines como salidas
	DDRD |= PINES_MOTORES;

	// PWM inicialmente en cero
	OCR0A = 0;
	OCR0B = 0;
	TCNT0 = 0;

	// Timer0 en modo Fast PWM de 8 bits
	TCCR0A = (1 << WGM01) |
	(1 << WGM00);

	// Prescaler de 64
	TCCR0B = (1 << CS01) |
	(1 << CS00);

	// Arrancar con motores detenidos
	velocidad_actual = 0;
	configurado = 0;

	motores_detener();

void motores_velocidad(uint8_t velocidad)
{
  // Guardar valor de PWM
	velocidad_actual = velocidad;
}

// =====================================
// MOVIMIENTO HACIA ADELANTE
// =====================================

void motores_avanzar(void)
{
	motores_aplicar(1, 1);
}


// =====================================
// MOVIMIENTO HACIA ATRAS
// =====================================

void motores_retroceder(void)
{
	motores_aplicar(-1, -1);
}

// =====================================
// GIRAR A LA IZQUIERDA
// =====================================

void motores_izquierda(void)
{
	// Motor izquierdo hacia atras
	PORTD &= ~((1 << IN1) | (1 << IN2));
	PORTD |= (1 << IN2);

	// Motor derecho hacia adelante
	PORTB &= ~((1 << IN3) | (1 << IN4));
	PORTB |= (1 << IN3);

	motores_aplicar(-1, 1);
}

// =====================================
// GIRAR A LA DERECHA
// =====================================

void motores_derecha(void)
{
	// Motor izquierdo hacia adelante
	PORTD &= ~((1 << IN1) | (1 << IN2));
	PORTD |= (1 << IN1);

	// Motor derecho hacia atras
	PORTB &= ~((1 << IN3) | (1 << IN4));
	PORTB |= (1 << IN4);

	motores_aplicar(1, -1);
}

// =====================================
// DETENER MOTORES
// =====================================

void motores_detener(void)
{
   motores_aplicar(0, 0);
}

