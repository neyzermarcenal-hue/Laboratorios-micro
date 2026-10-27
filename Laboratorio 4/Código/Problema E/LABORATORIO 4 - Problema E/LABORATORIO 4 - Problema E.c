
#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>

#define MPU_DIR 0x68
#define N_LEDS 64
#define N_DATOS (N_LEDS * 15)
#define MATRIZ_SERPENTINA 0

#define MARCA_CALIBRACION 0xE605
#define BUZZER_ACTIVO_BAJO 1

static uint8_t trama[N_DATOS];

typedef struct
{
	uint16_t marca;
	float offset_roll;
	float offset_pitch;
} calibracion_t;

calibracion_t EEMEM calibracion_eeprom;

// Inicializacion de UART
void uart_init(void)
{
	UBRR0H = 0;
	UBRR0L = 103;

	UCSR0B = (1 << TXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_texto(const char *s)
{
	while (*s)
	{
		while (!(UCSR0A & (1 << UDRE0)));
		UDR0 = *s++;
	}
}

// Inicializacion de I2C
void i2c_init(void)
{
	DDRC &= ~((1 << PC4) | (1 << PC5));
	PORTC &= ~((1 << PC4) | (1 << PC5));

	TWSR = 0;
	TWBR = 72;
	TWCR = (1 << TWEN);
}

uint8_t i2c_esperar(void)
{
	uint16_t limite = 60000;

	while (!(TWCR & (1 << TWINT)) && --limite);

	return limite != 0;
}

uint8_t i2c_inicio(uint8_t direccion)
{
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);

	if (!i2c_esperar())
	return 0;

	uint8_t estado = TWSR & 0xF8;

	if (estado != 0x08 && estado != 0x10)
	return 0;

	TWDR = direccion;
	TWCR = (1 << TWINT) | (1 << TWEN);

	if (!i2c_esperar())
	return 0;

	estado = TWSR & 0xF8;

	return estado == 0x18 || estado == 0x40;
}

uint8_t i2c_enviar(uint8_t dato)
{
	TWDR = dato;
	TWCR = (1 << TWINT) | (1 << TWEN);

	if (!i2c_esperar())
	return 0;

	return (TWSR & 0xF8) == 0x28;
}

uint8_t i2c_recibir(uint8_t *dato, uint8_t ack)
{
	TWCR = (1 << TWINT) | (1 << TWEN) |
	(ack ? (1 << TWEA) : 0);

	if (!i2c_esperar())
	return 0;

	*dato = TWDR;

	return (TWSR & 0xF8) == (ack ? 0x50 : 0x58);
}

void i2c_fin(void)
{
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}

// Escritura de registros del MPU6050
uint8_t mpu_escribir(uint8_t registro, uint8_t valor)
{
	uint8_t ok = i2c_inicio(MPU_DIR << 1);

	if (ok)
	ok = i2c_enviar(registro);

	if (ok)
	ok = i2c_enviar(valor);

	i2c_fin();

	return ok;
}

// Inicializacion del MPU6050
uint8_t mpu_init(void)
{
	_delay_ms(100);

	if (!mpu_escribir(0x6B, 0x00))
	return 0;

	if (!mpu_escribir(0x1C, 0x00))
	return 0;

	if (!mpu_escribir(0x1B, 0x00))
	return 0;

	return 1;
}

// Lectura de acelerometro y giroscopio
uint8_t mpu_leer(int16_t *ax, int16_t *ay, int16_t *az,
int16_t *gx, int16_t *gy)
{
	uint8_t datos[14];

	uint8_t ok = i2c_inicio(MPU_DIR << 1);

	if (ok)
	ok = i2c_enviar(0x3B);

	if (ok)
	ok = i2c_inicio((MPU_DIR << 1) | 1);

	if (ok)
	{
		for (uint8_t i = 0; i < 14; i++)
		{
			if (!i2c_recibir(&datos[i], i < 13))
			{
				ok = 0;
				break;
			}
		}
	}

	i2c_fin();

	if (!ok)
	return 0;

	*ax = (int16_t)(((uint16_t)datos[0] << 8) | datos[1]);
	*ay = (int16_t)(((uint16_t)datos[2] << 8) | datos[3]);
	*az = (int16_t)(((uint16_t)datos[4] << 8) | datos[5]);

	*gx = (int16_t)(((uint16_t)datos[8] << 8) | datos[9]);
	*gy = (int16_t)(((uint16_t)datos[10] << 8) | datos[11]);

	return 1;
}

// Inicializacion de la matriz RGB por SPI
void matriz_init(void)
{
	DDRB |= (1 << PB3) | (1 << PB5) | (1 << PB2);
	PORTB |= (1 << PB2);

	SPCR = (1 << SPE) | (1 << MSTR);
}

// Codificacion de colores para WS2812B
void ws_canal(uint8_t led, uint8_t canal, uint8_t valor)
{
	uint16_t inicio = ((uint16_t)led * 3 + canal) * 5;

	for (uint8_t i = 0; i < 5; i++)
	trama[inicio + i] = 0;

	for (uint8_t bit = 0; bit < 8; bit++)
	{
		uint8_t patron = (valor & (0x80 >> bit))
		? 0x1C : 0x10;

		for (uint8_t j = 0; j < 5; j++)
		{
			uint8_t posicion = bit * 5 + j;

			if (patron & (1 << (4 - j)))
			{
				trama[inicio + posicion / 8] |=
				(1 << (7 - posicion % 8));
			}
		}
	}
}

// Mostrar una burbuja en la matriz 8x8
void matriz_mostrar(uint8_t x, uint8_t y,
uint8_t rojo, uint8_t verde,
uint8_t azul)
{
	uint8_t columna = x;

	if (MATRIZ_SERPENTINA && (y & 1))
	columna = 7 - x;

	uint8_t seleccionado = y * 8 + columna;

	for (uint8_t led = 0; led < N_LEDS; led++)
	{
		uint8_t activo = (led == seleccionado);

		// Orden de colores GRB
		ws_canal(led, 0, activo ? verde : 0);
		ws_canal(led, 1, activo ? rojo : 0);
		ws_canal(led, 2, activo ? azul : 0);
	}

	for (uint16_t i = 0; i < N_DATOS; i++)
	{
		SPDR = trama[i];

		while (!(SPSR & (1 << SPIF)));
	}

	_delay_us(300);
}

// Apagar la matriz durante el parpadeo
void matriz_apagar(void)
{
	for (uint8_t led = 0; led < N_LEDS; led++)
	{
		ws_canal(led, 0, 0);
		ws_canal(led, 1, 0);
		ws_canal(led, 2, 0);
	}

	for (uint16_t i = 0; i < N_DATOS; i++)
	{
		SPDR = trama[i];

		while (!(SPSR & (1 << SPIF)));
	}

	_delay_us(300);
}

// Convertir inclinacion a posicion de 0 a 7
uint8_t angulo_a_pixel(float angulo)
{
	if (angulo < -30.0f)
	angulo = -30.0f;

	if (angulo > 30.0f)
	angulo = 30.0f;

	return (uint8_t)(((angulo + 30.0f) * 7.0f / 60.0f)
	+ 0.5f);
}

// Calcular angulos usando el acelerometro
void calcular_angulos(int16_t ax, int16_t ay, int16_t az,
float *roll, float *pitch)
{
	float fx = (float)ax;
	float fy = (float)ay;
	float fz = (float)az;

	*roll = atan2f(fy, fz) * 57.29578f;

	*pitch = atan2f(-fx, sqrtf(fy * fy + fz * fz))
	* 57.29578f;
}

// Seleccionar posicion y color de la burbuja
void color_burbuja(float roll, float pitch,
uint8_t *x, uint8_t *y,
uint8_t *rojo, uint8_t *verde,
uint8_t *azul)
{
	float ar = fabsf(roll);
	float ap = fabsf(pitch);

	*x = angulo_a_pixel(roll);
	*y = angulo_a_pixel(pitch);

	if (ar <= 3.0f && ap <= 3.0f)
	{
		*x = 3;
		*y = 3;

		*rojo = 0;
		*verde = 0;
		*azul = 16;
	}
	else if (ar > 3.0f && ap > 3.0f &&
	fabsf(ar - ap) <= 5.0f)
	{
		*rojo = 16;
		*verde = 16;
		*azul = 0;
	}
	else if (ar >= ap)
	{
		*rojo = 16;
		*verde = 0;
		*azul = 0;
	}
	else
	{
		*rojo = 0;
		*verde = 16;
		*azul = 0;
	}
}

// Timer1 para medir el tiempo entre lecturas
void timer1_init(void)
{
	TCCR1A = 0;
	TCCR1B = (1 << CS11) | (1 << CS10);
	TCNT1 = 0;
}

float periodo_muestreo(void)
{
	uint16_t tiempo = TCNT1;
	TCNT1 = 0;

	float periodo = (float)tiempo * 0.000004f;

	if (periodo < 0.005f || periodo > 0.25f)
	periodo = 0.1f;

	return periodo;
}

// Combinar acelerometro y giroscopio
void actualizar_filtro(int16_t ax, int16_t ay, int16_t az,
int16_t gx, int16_t gy,
float periodo,
float *roll, float *pitch,
uint8_t *primera_lectura)
{
	float roll_acelerometro;
	float pitch_acelerometro;

	calcular_angulos(ax, ay, az,
	&roll_acelerometro,
	&pitch_acelerometro);

	if (*primera_lectura)
	{
		*roll = roll_acelerometro;
		*pitch = pitch_acelerometro;
		*primera_lectura = 0;
	}
	else
	{
		*roll = 0.96f * (*roll +
		((float)gx / 131.0f) * periodo)
		+ 0.04f * roll_acelerometro;

		*pitch = 0.96f * (*pitch +
		((float)gy / 131.0f) * periodo)
		+ 0.04f * pitch_acelerometro;
	}
}

// Detectar pulsaciones de los botones
uint8_t boton_pulsado(uint8_t pin, uint8_t *estado_anterior)
{
	uint8_t presionado = !(PIND & (1 << pin));
	uint8_t pulsacion = presionado && !(*estado_anterior);

	*estado_anterior = presionado;

	return pulsacion;
}

// Inicializar el buzzer
void buzzer_init(void)
{
	DDRD |= (1 << PD4);

	if (BUZZER_ACTIVO_BAJO)
	PORTD |= (1 << PD4);
	else
	PORTD &= ~(1 << PD4);
}

// Emitir un pitido al llegar al limite
void buzzer_pitido(void)
{
	if (BUZZER_ACTIVO_BAJO)
	PORTD &= ~(1 << PD4);
	else
	PORTD |= (1 << PD4);

	_delay_ms(80);

	if (BUZZER_ACTIVO_BAJO)
	PORTD |= (1 << PD4);
	else
	PORTD &= ~(1 << PD4);
}

// Programa principal
int main(void)
{
	int16_t ax, ay, az;
	int16_t gx, gy;

	float roll = 0.0f;
	float pitch = 0.0f;

	uint8_t x = 3;
	uint8_t y = 3;

	uint8_t rojo = 0;
	uint8_t verde = 0;
	uint8_t azul = 16;

	char mensaje[64];

	uint8_t primera_lectura = 1;
	uint8_t hold = 0;
	uint8_t estado_anterior_hold = 0;
	uint8_t estado_anterior_calibrar = 0;

	uint8_t x_anterior = 255;
	uint8_t y_anterior = 255;
	uint8_t rojo_anterior = 255;
	uint8_t verde_anterior = 255;
	uint8_t azul_anterior = 255;

	float offset_roll = 0.0f;
	float offset_pitch = 0.0f;

	uint8_t borde_anterior = 0;
	uint8_t contador_parpadeo = 0;
	uint8_t mostrar_burbuja = 1;
	uint8_t mostrar_anterior = 255;

	calibracion_t calibracion;

	DDRD &= ~((1 << PD2) | (1 << PD3));
	PORTD |= (1 << PD2) | (1 << PD3);

	buzzer_init();
	uart_init();
	i2c_init();
	matriz_init();

	// Mostrar LED inicial
	matriz_mostrar(3, 3, 0, 0, 16);

	if (mpu_init())
	uart_texto("MPU6050 inicializado\r\n");
	else
	uart_texto("ERROR MPU6050\r\n");

	// Recuperar la calibracion de EEPROM
	eeprom_read_block(&calibracion,
	&calibracion_eeprom,
	sizeof(calibracion));

	if (calibracion.marca == MARCA_CALIBRACION &&
	fabsf(calibracion.offset_roll) <= 180.0f &&
	fabsf(calibracion.offset_pitch) <= 90.0f)
	{
		offset_roll = calibracion.offset_roll;
		offset_pitch = calibracion.offset_pitch;
	}

	timer1_init();

	while (1)
	{
		float periodo = periodo_muestreo();

		if (mpu_leer(&ax, &ay, &az, &gx, &gy))
		{
			actualizar_filtro(ax, ay, az, gx, gy,
			periodo, &roll, &pitch,
			&primera_lectura);

			// Alternar HOLD
			if (boton_pulsado(PD2, &estado_anterior_hold))
			hold = !hold;

			// Guardar la posicion actual como nivel cero
			if (boton_pulsado(PD3, &estado_anterior_calibrar))
			{
				offset_roll = roll;
				offset_pitch = pitch;

				calibracion.marca = MARCA_CALIBRACION;
				calibracion.offset_roll = offset_roll;
				calibracion.offset_pitch = offset_pitch;

				eeprom_update_block(&calibracion,
				&calibracion_eeprom,
				sizeof(calibracion));

				hold = 0;

				uart_texto("Calibracion guardada\r\n");
			}

			float roll_nivelado = roll - offset_roll;
			float pitch_nivelado = pitch - offset_pitch;

			if (!hold)
			{
				color_burbuja(roll_nivelado, pitch_nivelado,
				&x, &y,
				&rojo, &verde, &azul);
			}

			// Detectar cuando la burbuja alcanza un borde
			uint8_t borde = (x == 0 || x == 7 ||
			y == 0 || y == 7);

			if (!hold && borde && !borde_anterior)
			buzzer_pitido();

			borde_anterior = borde;

			// Parpadeo cuando alcanza el limite
			if (!hold && borde)
			{
				contador_parpadeo++;

				if (contador_parpadeo >= 3)
				{
					contador_parpadeo = 0;
					mostrar_burbuja = !mostrar_burbuja;
				}
			}
			else if (!hold)
			{
				contador_parpadeo = 0;
				mostrar_burbuja = 1;
			}

			// Actualizar solo si cambia la imagen
			if (!hold &&
			(x != x_anterior || y != y_anterior ||
			rojo != rojo_anterior ||
			verde != verde_anterior ||
			azul != azul_anterior ||
			mostrar_burbuja != mostrar_anterior))
			{
				if (mostrar_burbuja)
				matriz_mostrar(x, y, rojo, verde, azul);
				else
				matriz_apagar();

				x_anterior = x;
				y_anterior = y;
				rojo_anterior = rojo;
				verde_anterior = verde;
				azul_anterior = azul;
				mostrar_anterior = mostrar_burbuja;
			}

			snprintf(mensaje, sizeof(mensaje),
			"ROLL:%d PITCH:%d %s\r\n",
			(int)roll_nivelado,
			(int)pitch_nivelado,
			hold ? "HOLD" : "NORMAL");

			uart_texto(mensaje);
		}
		else
		{
			uart_texto("ERROR lectura I2C\r\n");
		}

		_delay_ms(100);
	}
}
