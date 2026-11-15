
#include <avr/io.h>
#include "motores.h"

// =====================================
// DEFINICION DE PINES DEL L298N
// =====================================

// Motor A (izquierdo)
#define ENA PD5
#define IN1 PD4
#define IN2 PD7

// Motor B (derecho)
#define ENB PD6
#define IN3 PB0
#define IN4 PB4

// =====================================
// INICIALIZACION DE LOS MOTORES
// =====================================

void motores_init(void)
{
    // Configurar pines como salidas
    DDRD |= (1 << ENA) | (1 << IN1)
          | (1 << IN2) | (1 << ENB);

    DDRB |= (1 << IN3) | (1 << IN4);

      // Detener los motores inicialmente
      motores_detener();


    // Timer0 en modo Fast PWM
    // PWM en PD5 (OC0B) y PD6 (OC0A)
    TCCR0A = (1 << COM0A1) | (1 << COM0B1)
           | (1 << WGM01) | (1 << WGM00);

    // Prescaler de 64
    TCCR0B = (1 << CS01) | (1 << CS00);

    // Velocidad inicial aproximada: 70%
    motores_velocidad(180);
}

// =====================================
// CONTROL DE VELOCIDAD
// =====================================

void motores_velocidad(uint8_t velocidad)
{
    OCR0A = velocidad;  // Motor B
    OCR0B = velocidad;  // Motor A
}

// =====================================
// MOVIMIENTO HACIA ADELANTE
// =====================================

void motores_avanzar(void)
{
	PORTD &= ~((1 << IN1) | (1 << IN2));
	PORTB &= ~((1 << IN3) | (1 << IN4));

	PORTD |= (1 << IN1);
	PORTB |= (1 << IN3);
}


// =====================================
// MOVIMIENTO HACIA ATRAS
// =====================================

void motores_retroceder(void)
{
	PORTD &= ~((1 << IN1) | (1 << IN2));
	PORTB &= ~((1 << IN3) | (1 << IN4));

	PORTD |= (1 << IN2);
	PORTB |= (1 << IN4);
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
}

// =====================================
// DETENER MOTORES
// =====================================

void motores_detener(void)
{
    PORTD &= ~((1 << IN1) | (1 << IN2));
    PORTB &= ~((1 << IN3) | (1 << IN4));
}

