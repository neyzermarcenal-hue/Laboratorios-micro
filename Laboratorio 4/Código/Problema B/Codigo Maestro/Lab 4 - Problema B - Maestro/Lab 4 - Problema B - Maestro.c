
#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

// Laboratorio 4 - Problema B: Maestro SPI y ADC
// SPI basado en los ejemplos de Yisus
// https://lord-jebus.github.io/TMI-UTEC/AVR-GCC/spi.html
// ADC basado en los ejemplos de Yisus
// https://lord-jebus.github.io/TMI-UTEC/AVR-GCC/adc.html

#define START 0x01
#define STOP  0x02
#define VELOCIDAD 0x03

// Configurar SPI como maestro.
void spi_master_init(void)
{
	// Configurar MOSI, SCK y SS como salidas.
	DDRB |= (1 << DDB3) | (1 << DDB5) | (1 << DDB2);

	// Configurar MISO como entrada.
	DDRB &= ~(1 << DDB4);

	// Mantener el esclavo deseleccionado.
	PORTB |= (1 << PB2);

	// Habilitar SPI, modo maestro, clock F_CPU/16.
	SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0);
}

// Transferir un byte por SPI.
uint8_t spi_transfer(uint8_t data)
{
	SPDR = data;

	// Esperar a que termine la transferencia.
	while (!(SPSR & (1 << SPIF)));

	return SPDR;
}

// Enviar una trama de dos bytes: COMANDO y VALOR.
void spi_send_command(uint8_t comando, uint8_t valor)
{
	PORTB &= ~(1 << PB2); // Seleccionar esclavo.

	spi_transfer(comando);
	_delay_us(100);
	spi_transfer(valor);

	PORTB |= (1 << PB2); // Deseleccionar esclavo.
}

// Inicializar ADC. Basado en el ejemplo de Yisus.
void adc_init(void)
{
	// Referencia AVcc, seleccion inicial ADC0.
	ADMUX = (1 << REFS0);

	// Habilitar ADC, prescaler de 128.
	// 16MHz/128 = 125kHz.
	ADCSRA = (1 << ADEN) | (1 << ADPS2) |
	(1 << ADPS1) | (1 << ADPS0);
}

// Leer el valor ADC de un canal.
uint16_t adc_read(uint8_t channel)
{
	// Seleccionar canal sin modificar la referencia.
	ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);

	// Iniciar conversion.
	ADCSRA |= (1 << ADSC);

	// Esperar fin de conversion.
	while (ADCSRA & (1 << ADSC));

	// Devolver resultado de 10 bits.
	return ADC;
}

int main(void)
{
	uint8_t marcha = 0;
	uint8_t velocidad;

	// Configurar pulsador PD3 como entrada con pull-up.
	DDRD &= ~(1 << PD3);
	PORTD |= (1 << PD3);

	spi_master_init();
	adc_init();

	_delay_ms(200);
	spi_send_command(STOP, 0);

	while (1)
	{
		if (!(PIND & (1 << PD3)))
		{
			_delay_ms(25); // Antirrebote.

			if (!(PIND & (1 << PD3)))
			{
				marcha = !marcha;

				if (marcha)
				spi_send_command(START, 0);
				else
				spi_send_command(STOP, 0);

				// Esperar a que se suelte el boton.
				while (!(PIND & (1 << PD3)));
				_delay_ms(25);
			}
		}

		if (marcha)
		{
			// Leer ADC1 y convertir de 10 a 8 bits.
			velocidad = adc_read(1) >> 2;

			// Enviar velocidad al esclavo.
			spi_send_command(VELOCIDAD, velocidad);
		}

		_delay_ms(75);
	}
}
