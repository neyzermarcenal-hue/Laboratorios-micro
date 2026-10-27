
#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdio.h>

// Laboratorio 4 - Problema B: Maestro SPI, ADC, DHT11 y LCD
// SPI basado en los ejemplos de Yisus
// https://lord-jebus.github.io/TMI-UTEC/AVR-GCC/spi.html
// ADC basado en los ejemplos de Yisus
// https://lord-jebus.github.io/TMI-UTEC/AVR-GCC/adc.html
// DHT11 basado en los ejemplos de Yisus
// https://lord-jebus.github.io/TMI-UTEC/AVR-GCC/dht-11.html
// LCD I2C adaptado del material LCD proporcionado en clase.

#define START 0x01
#define STOP  0x02
#define VELOCIDAD 0x03
#define PISTON 0x04
#define PARO_EMERGENCIA 0x05
#define REINICIAR 0x06

// Umbrales de la LDR para detectar piezas.
#define LDR_PIEZA 400
#define LDR_LIBRE 550

// Umbrales iniciales de temperatura (grados Celsius).
#define TEMP_MAX 28
#define TEMP_REINICIO 27

// Cambiar a 0x3F si la LCD tiene otra direccion I2C.
#define LCD_ADDR 0x27

// Configuracion del DHT11 en PD2.
#define DHT11_PIN PD2
#define DHT11_PORT PORTD
#define DHT11_DDR DDRD
#define DHT11_PINR PIND

volatile uint16_t contador_piezas = 0;
uint8_t ultimo_comando = STOP;
uint8_t lcd_disponible = 0;

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

	// Recordar ultimo comando importante para la LCD.
	if (comando != VELOCIDAD)
	ultimo_comando = comando;
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

// Inicializar pin del DHT11. Basado en Yisus.
void dht11_init(void)
{
	DHT11_DDR &= ~(1 << DHT11_PIN);
	DHT11_PORT |= (1 << DHT11_PIN);
}

// Enviar senal de inicio al DHT11.
void dht11_start(void)
{
	DHT11_DDR |= (1 << DHT11_PIN);
	DHT11_PORT &= ~(1 << DHT11_PIN);
	_delay_ms(18);
	DHT11_PORT |= (1 << DHT11_PIN);
	_delay_us(30);
	DHT11_DDR &= ~(1 << DHT11_PIN);
}

// Esperar estado del DHT11 con limite para evitar bloqueos.
uint8_t dht11_wait_state(uint8_t estado)
{
	uint8_t tiempo = 0;

	while (!!(DHT11_PINR & (1 << DHT11_PIN)) != estado)
	{
		_delay_us(1);
		tiempo++;

		if (tiempo >= 120)
		return 0;
	}

	return 1;
}

// Esperar respuesta del DHT11.
uint8_t dht11_check_response(void)
{
	if (!dht11_wait_state(0))
	return 0;

	if (!dht11_wait_state(1))
	return 0;

	if (!dht11_wait_state(0))
	return 0;

	return 1;
}

// Leer un bit del DHT11.
uint8_t dht11_read_bit(uint8_t *bit)
{
	if (!dht11_wait_state(1))
	return 0;

	_delay_us(35);

	// Leer valor del bit.
	*bit = !!(DHT11_PINR & (1 << DHT11_PIN));

	if (!dht11_wait_state(0))
	return 0;

	return 1;
}

// Leer un byte del DHT11.
uint8_t dht11_read_byte(uint8_t *byte)
{
	uint8_t bit;

	*byte = 0;

	for (uint8_t i = 0; i < 8; i++)
	{
		if (!dht11_read_bit(&bit))
		return 0;

		*byte <<= 1;
		*byte |= bit;
	}

	return 1;
}

// Leer temperatura y humedad con verificacion de checksum.
uint8_t dht11_read(uint8_t *humidity, uint8_t *temperature)
{
	uint8_t data[5] = {0};
	uint8_t checksum;

	dht11_start();

	if (!dht11_check_response())
	return 0;

	// Leer los cinco bytes enviados por el sensor.
	for (uint8_t i = 0; i < 5; i++)
	{
		if (!dht11_read_byte(&data[i]))
		return 0;
	}

	// Verificar checksum.
	checksum = data[0] + data[1] + data[2] + data[3];

	if (checksum != data[4])
	return 0;

	*humidity = data[0];
	*temperature = data[2];

	return 1;
}

// Inicializar I2C en 100 kHz para la LCD PCF8574.
void lcd_i2c_init(void)
{
	TWSR = 0;
	TWBR = 72;
	TWCR = (1 << TWEN);
}

// Transferir un byte por I2C; devolver 0 si no hay respuesta.
uint8_t lcd_i2c_write(uint8_t dato)
{
	uint16_t espera;
	uint8_t estado;

	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
	espera = 60000;

	while (!(TWCR & (1 << TWINT)) && --espera);

	if (!espera)
	return 0;

	TWDR = (LCD_ADDR << 1);
	TWCR = (1 << TWINT) | (1 << TWEN);
	espera = 60000;

	while (!(TWCR & (1 << TWINT)) && --espera);

	if (!espera)
	return 0;

	estado = TWSR & 0xF8;

	if (estado != 0x18)
	{
		TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
		return 0;
	}

	TWDR = dato;
	TWCR = (1 << TWINT) | (1 << TWEN);
	espera = 60000;

	while (!(TWCR & (1 << TWINT)) && --espera);

	if (!espera)
	return 0;

	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);

	return 1;
}

// Enviar cuatro bits al LCD por el expansor PCF8574.
void lcd_nibble(uint8_t dato)
{
	uint8_t base = dato | 0x08; // Luz de fondo.

	lcd_i2c_write(base | 0x04); // EN HIGH.
	_delay_us(2);

	lcd_i2c_write(base); // EN LOW.
	_delay_us(50);
}

// Enviar comando o caracter al LCD.
void lcd_send(uint8_t dato, uint8_t rs)
{
	lcd_nibble((dato & 0xF0) | rs);
	lcd_nibble(((dato << 4) & 0xF0) | rs);
}

void lcd_cmd(uint8_t dato)
{
	lcd_send(dato, 0);

	if (dato == 0x01)
	_delay_ms(2);
}

void lcd_char(char caracter)
{
	lcd_send((uint8_t)caracter, 1);
}

void lcd_texto(const char *texto)
{
	while (*texto)
	lcd_char(*texto++);
}

// Inicializar LCD 16x2 en modo de cuatro bits.
void lcd_init(void)
{
	lcd_i2c_init();
	_delay_ms(50);

	lcd_disponible = lcd_i2c_write(0x08);

	if (!lcd_disponible)
	return;

	lcd_nibble(0x30);
	_delay_ms(5);

	lcd_nibble(0x30);
	_delay_us(150);

	lcd_nibble(0x30);
	lcd_nibble(0x20);

	lcd_cmd(0x28);
	lcd_cmd(0x0C);
	lcd_cmd(0x06);
	lcd_cmd(0x01);
}

// Escribir una linea completa sin dejar caracteres viejos.
void lcd_linea(uint8_t fila, const char *texto)
{
	if (!lcd_disponible)
	return;

	lcd_cmd(fila ? 0xC0 : 0x80);

	uint8_t i = 0;

	while (*texto && i < 16)
	{
		lcd_char(*texto++);
		i++;
	}

	while (i++ < 16)
	lcd_char(' ');
}

// Mostrar sensores, contador, estado y ultimo comando.
void lcd_mostrar(uint16_t piezas, uint8_t velocidad, uint16_t luz,
uint8_t temp, uint8_t hum, uint8_t dht_ok,
uint8_t marcha, uint8_t emergencia, uint8_t pagina)
{
	char linea[17];
	const char *cmd = "STOP";

	if (ultimo_comando == START)
	cmd = "START";

	if (ultimo_comando == PISTON)
	cmd = "PISTON";

	if (ultimo_comando == PARO_EMERGENCIA)
	cmd = "EMERG";

	if (ultimo_comando == REINICIAR)
	cmd = "RESET";

	if (pagina == 0)
	{
		snprintf(linea, sizeof(linea), "P:%u V:%u%%",
		piezas, (uint16_t)velocidad * 100 / 255);
		lcd_linea(0, linea);

		if (emergencia)
		snprintf(linea, sizeof(linea), "ALARMA %s", cmd);
		else if (marcha)
		snprintf(linea, sizeof(linea), "MARCHA %s", cmd);
		else
		snprintf(linea, sizeof(linea), "PARADO %s", cmd);

		lcd_linea(1, linea);
	}
	else
	{
		if (dht_ok)
		snprintf(linea, sizeof(linea),
		"T:%uC H:%u%%", temp, hum);
		else
		snprintf(linea, sizeof(linea), "DHT SIN LECTURA");

		lcd_linea(0, linea);

		snprintf(linea, sizeof(linea), "LDR:%u", luz);
		lcd_linea(1, linea);
	}
}

int main(void)
{
	uint8_t marcha = 0;
	uint8_t velocidad = 0;
	uint8_t pieza_detectada = 0;
	uint16_t luz = 0;

	uint8_t temperatura = 0;
	uint8_t humedad = 0;
	uint8_t emergencia = 0;
	uint8_t lectura_valida = 0;
	uint8_t tiempo_dht = 0;
	uint8_t tiempo_lcd = 0;
	uint8_t pagina_lcd = 0;
	uint8_t tiempo_pagina = 0;

	// Configurar pulsador PD3 como entrada con pull-up.
	DDRD &= ~(1 << PD3);
	PORTD |= (1 << PD3);

	spi_master_init();
	adc_init();
	dht11_init();
	lcd_init();

	_delay_ms(200);
	spi_send_command(STOP, 0);

	while (1)
	{
		if (!(PIND & (1 << PD3)))
		{
			_delay_ms(25); // Antirrebote.

			if (!(PIND & (1 << PD3)))
			{
				if (emergencia)
				{
					// Recuperar solo con temperatura segura.
					if (lectura_valida &&
					temperatura < TEMP_REINICIO)
					{
						emergencia = 0;
						marcha = 0;
						spi_send_command(REINICIAR, 0);
					}
				}
				else
				{
					marcha = !marcha;

					if (marcha)
					spi_send_command(START, 0);
					else
					spi_send_command(STOP, 0);
				}

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

			// Leer la LDR conectada al canal ADC0.
			luz = adc_read(0);

			// Detectar una nueva pieza.
			if (!pieza_detectada && luz < LDR_PIEZA)
			{
				pieza_detectada = 1;
				contador_piezas++;

				// Enviar comando para accionar el piston.
				spi_send_command(PISTON, 1);
			}

			// Esperar a que la pieza deje de tapar la LDR.
			if (pieza_detectada && luz > LDR_LIBRE)
			{
				pieza_detectada = 0;
			}
		}
		else
		{
			velocidad = 0;
			luz = adc_read(0);
		}

		// Leer el DHT11 aproximadamente cada 2 segundos.
		tiempo_dht++;

		if (tiempo_dht >= 27)
		{
			tiempo_dht = 0;

			if (dht11_read(&humedad, &temperatura))
			{
				lectura_valida = 1;

				// Activar emergencia por temperatura elevada.
				if (!emergencia && temperatura > TEMP_MAX)
				{
					emergencia = 1;
					marcha = 0;
					spi_send_command(PARO_EMERGENCIA, temperatura);
				}
			}
			else
			{
				lectura_valida = 0;
				// Mostrar error de lectura en la LCD.
			}
		}

		// Actualizar LCD y alternar las paginas.
		tiempo_lcd++;
		tiempo_pagina++;

		if (tiempo_pagina >= 27)
		{
			tiempo_pagina = 0;
			pagina_lcd = !pagina_lcd;
		}

		if (tiempo_lcd >= 6)
		{
			tiempo_lcd = 0;

			lcd_mostrar(contador_piezas, velocidad, luz,
			temperatura, humedad, lectura_valida,
			marcha, emergencia, pagina_lcd);
		}

		_delay_ms(75);
	}
}
