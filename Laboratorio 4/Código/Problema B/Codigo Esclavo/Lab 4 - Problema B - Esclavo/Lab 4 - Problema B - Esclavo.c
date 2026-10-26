
#define F_CPU 16000000UL

#include <avr/io.h>
#include <stdint.h>

// Laboratorio 4 - Problema B: Esclavo SPI
// SPI basado en los ejemplos de Yisus
// https://lord-jebus.github.io/TMI-UTEC/AVR-GCC/spi.html

#define START 0x01
#define STOP  0x02

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

int main(void)
{
	uint8_t comando;
	uint8_t valor;

	// Configurar LED verde en PD2 como salida.
	DDRD |= (1 << PD2);
	PORTD &= ~(1 << PD2);

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
			PORTD |= (1 << PD2);
			break;

			case STOP:
			PORTD &= ~(1 << PD2);
			break;

			default:
			break;
		}
	}
}
