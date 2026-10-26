
#define F_CPU 16000000UL

#include <avr/io.h>
#include <stdint.h>

// Laboratorio 4 - Problema B: Esclavo SPI y PWM
// SPI basado en los ejemplos de Yisus
// https://lord-jebus.github.io/TMI-UTEC/AVR-GCC/spi.html
// PWM basado en los ejemplos de Yisus
// https://lord-jebus.github.io/TMI-UTEC/AVR-GCC/pwm.html

#define START 0x01
#define STOP  0x02
#define VELOCIDAD 0x03

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

int main(void)
{
	uint8_t comando;
	uint8_t valor;
	uint8_t marcha = 0;

	// Configurar LED verde en PD2 como salida.
	DDRD |= (1 << PD2);
	PORTD &= ~(1 << PD2);

	motor_init();
	spi_slave_init();

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
			break;

			case VELOCIDAD:
			// Actualizar PWM solo si esta en marcha.
			if (marcha)
			OCR0B = valor;
			else
			OCR0B = 0;
			break;

			default:
			break;
		}
	}
}
