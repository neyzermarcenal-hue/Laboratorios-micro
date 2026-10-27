
#define F_CPU 16000000UL

#include <avr/io.h>
#include <stdint.h>
#include <avr/interrupt.h>

// Laboratorio 4 - Problema B: Esclavo SPI y PWM
// SPI basado en los ejemplos de Yisus
// https://lord-jebus.github.io/TMI-UTEC/AVR-GCC/spi.html
// PWM basado en los ejemplos de Yisus
// https://lord-jebus.github.io/TMI-UTEC/AVR-GCC/pwm.html

#define START 0x01
#define STOP  0x02
#define VELOCIDAD 0x03
#define PISTON 0x04

volatile uint8_t piston_activo = 0;
volatile uint8_t piston_tiempo = 0;

// Configurar SPI como esclavo.
void spi_slave_init(void)
{
	// Configurar MISO como salida.
	DDRB |= (1 << DDB4);

	// Configurar MOSI, SCK y SS como entradas.
	DDRB &= ~((1 << DDB3) | (1 << DDB5) | (1 << DDB2));

	// Habilitar SPI.
	SPCR = (1 << SPE);
}

// Recibir un byte desde el maestro.
uint8_t spi_slave_receive(void)
{
	// Esperar a que se complete la recepcion.
	while (!(SPSR & (1 << SPIF)));

	return SPDR;
}

// Inicializar PWM para controlar el motor.
void motor_init(void)
{
	// ENA en PD5, IN1 en PD7 e IN2 en PB0.
	DDRD |= (1 << PD5) | (1 << PD7);
	DDRB |= (1 << PB0);

	// Timer0 en Fast PWM, salida OC0B (PD5).
	TCCR0A = (1 << COM0B1) | (1 << WGM01) | (1 << WGM00);

	// Prescaler de 64.
	TCCR0B = (1 << CS01) | (1 << CS00);

	// Motor inicialmente apagado.
	OCR0B = 0;
	PORTD &= ~(1 << PD7);
	PORTB &= ~(1 << PB0);
}

// Detener el motor.
void motor_stop(void)
{
	OCR0B = 0;
	PORTD &= ~(1 << PD7);
	PORTB &= ~(1 << PB0);
}

// Inicializar PWM del servomotor SG90.
void servo_init(void)
{
	// PB1 (OC1A) como salida.
	DDRB |= (1 << PB1);

	// Timer1 en Fast PWM, con TOP en ICR1.
	TCCR1A = (1 << COM1A1) | (1 << WGM11);

	// Prescaler de 8.
	TCCR1B = (1 << WGM13) | (1 << WGM12) |
	(1 << CS11);

	// Periodo de 20 ms (50 Hz).
	ICR1 = 39999;

	// Posicion inicial del servo (pulso de 1 ms).
	OCR1A = 2000;

	// Timer2 en modo CTC.
	TCCR2A = (1 << WGM21);

	// Prescaler de 1024.
	TCCR2B = (1 << CS22) | (1 << CS21) |
	(1 << CS20);

	// Interrupcion aproximadamente cada 10 ms.
	OCR2A = 155;
	TIMSK2 |= (1 << OCIE2A);
}

// Interrupcion para regresar el piston a reposo.
ISR(TIMER2_COMPA_vect)
{
	if (piston_activo)
	{
		piston_tiempo++;

		// 40 intervalos de 10 ms = 400 ms.
		if (piston_tiempo >= 40)
		{
			OCR1A = 2000;
			piston_activo = 0;
		}
	}
}

int main(void)
{
	uint8_t comando;
	uint8_t valor;
	uint8_t marcha = 0;

	// Configurar LED verde en PD2 como salida.
	DDRD |= (1 << PD2);
	PORTD &= ~(1 << PD2);

	motor_init();
	servo_init();
	spi_slave_init();

	// Habilitar interrupciones.
	sei();

	while (1)
	{
		// Recibir los dos bytes enviados por el maestro.
		comando = spi_slave_receive();
		valor = spi_slave_receive();

		switch (comando)
		{
			case START:
			if (valor == 0)
			{
				marcha = 1;

				// Configurar sentido de giro.
				PORTD |= (1 << PD7);
				PORTB &= ~(1 << PB0);

				PORTD |= (1 << PD2);
			}
			break;

			case STOP:
			marcha = 0;
			motor_stop();
			PORTD &= ~(1 << PD2);

			// Regresar el piston a reposo.
			cli();
			piston_activo = 0;
			piston_tiempo = 0;
			OCR1A = 2000;
			sei();
			break;

			case VELOCIDAD:
			// Actualizar PWM solo si esta en marcha.
			if (marcha)
			OCR0B = valor;
			else
			OCR0B = 0;
			break;

			case PISTON:
			if (marcha && valor == 1)
			{
				// Mover el servo durante 400 ms.
				cli();
				piston_tiempo = 0;
				piston_activo = 1;
				OCR1A = 4000;
				sei();
			}
			break;

			default:
			break;
		}
	}
}
