/*
 * Problema A.c
 *
 * Created: 12/10/2026 0:53:40
 * Author : cvarg
 */ 
#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// Direccion de la pantalla LCD
#define LCD_I2C_ADDR 0x27

// Limites del sensor y margen de seguridad
#define MAX_SENSOR_TEMP 50.0f
#define SAFETY_MARGIN 20.0f

#define LCD_BACKLIGHT 0x08
#define LCD_EN        0x04
#define LCD_RS        0x01

// DHT11 conectado al pin digital 2 (PD2)
#define DHT_DDR   DDRD
#define DHT_PORT  PORTD
#define DHT_PINR  PIND
#define DHT_BIT   PD2

volatile uint8_t tomar_muestra = 0;
float temperatura_objetivo = 20.0f;
float temperatura = 0.0f;
uint8_t humedad = 0;

typedef enum {
	CALEFACTOR,
	IDEAL,
	VENTILADOR_BAJO,
	VENTILADOR_MEDIO,
	VENTILADOR_ALTO
} EstadoSistema;

EstadoSistema estado_actual = IDEAL;

// -------------------- I2C --------------------

void iniciar_i2c(void) {
	TWSR = 0x00;
	TWBR = 72; // Velocidad de 100 kHz
}

void iniciar_comunicacion(void) {
	uint16_t timeout = 10000;
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)) && --timeout);
}

void terminar_comunicacion(void) {
	TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
	_delay_us(100);
}

void enviar_i2c(uint8_t dato) {
	uint16_t timeout = 10000;
	TWDR = dato;
	TWCR = (1 << TWINT) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)) && --timeout);
}

// -------------------- PANTALLA LCD --------------------

void enviar_medio_byte(uint8_t dato) {
	iniciar_comunicacion();
	enviar_i2c(LCD_I2C_ADDR << 1);
	enviar_i2c(dato | LCD_EN);
	_delay_us(2);
	enviar_i2c(dato & ~LCD_EN);
	_delay_us(50);
	terminar_comunicacion();
}

void enviar_dato_lcd(uint8_t dato, uint8_t modo) {
	uint8_t parte_alta = dato & 0xF0;
	uint8_t parte_baja = (dato << 4) & 0xF0;

	enviar_medio_byte(parte_alta | modo | LCD_BACKLIGHT);
	enviar_medio_byte(parte_baja | modo | LCD_BACKLIGHT);
}

void mandar_comando_lcd(uint8_t comando) {
	enviar_dato_lcd(comando, 0);
	if (comando == 0x01 || comando == 0x02) {
		_delay_ms(2);
	}
}

void mostrar_caracter(char caracter) {
	enviar_dato_lcd(caracter, LCD_RS);
}

void mostrar_texto(const char *texto) {
	while (*texto) {
		mostrar_caracter(*texto++);
	}
}

void iniciar_lcd(void) {
	iniciar_i2c();
	_delay_ms(50);

	// Inicializar la pantalla en modo de 4 bits
	enviar_medio_byte(0x30 | LCD_BACKLIGHT);
	_delay_ms(5);
	enviar_medio_byte(0x30 | LCD_BACKLIGHT);
	_delay_us(150);
	enviar_medio_byte(0x30 | LCD_BACKLIGHT);
	_delay_us(150);
	enviar_medio_byte(0x20 | LCD_BACKLIGHT);
	_delay_ms(2);

	mandar_comando_lcd(0x28); // Dos lineas
	mandar_comando_lcd(0x0C); // Pantalla encendida, cursor apagado
	mandar_comando_lcd(0x06); // Avanzar el cursor
	mandar_comando_lcd(0x01); // Limpiar pantalla
	_delay_ms(2);
}

void ubicar_cursor(uint8_t fila, uint8_t columna) {
	uint8_t direccion = (fila == 0) ? (0x80 + columna) : (0xC0 + columna);
	mandar_comando_lcd(direccion);
}

// -------------------- COMUNICACION SERIAL --------------------

void iniciar_serial(unsigned int velocidad) {
	unsigned int valor = F_CPU / 16 / velocidad - 1;
	UBRR0H = (unsigned char)(valor >> 8);
	UBRR0L = (unsigned char)valor;
	UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8 bits, sin paridad, 1 bit de parada
}

void enviar_caracter(char caracter) {
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = caracter;
}

void enviar_texto(const char *texto) {
	while (*texto) {
		enviar_caracter(*texto++);
	}
}

// Pasar un numero decimal a texto sin usar printf
void enviar_decimal(float numero, void (*salida)(char)) {
	if (numero < 0) {
		salida('-');
		numero = -numero;
	}

	uint16_t entero = (uint16_t)numero;
	uint8_t decimal = (uint8_t)((numero - (float)entero) * 10.0f + 0.5f);

	if (decimal >= 10) {
		decimal = 0;
		entero++;
	}

	if (entero >= 100) {
		salida('0' + (entero / 100));
		salida('0' + ((entero / 10) % 10));
		} else if (entero >= 10) {
		salida('0' + (entero / 10));
	}

	salida('0' + (entero % 10));
	salida('.');
	salida('0' + decimal);
}

// Pasar un numero entero a texto
void enviar_entero(uint8_t numero, void (*salida)(char)) {
	if (numero >= 100) salida('0' + (numero / 100));
	if (numero >= 10) salida('0' + ((numero / 10) % 10));
	salida('0' + (numero % 10));
}

void enviar_decimal_serial(float numero) {
	enviar_decimal(numero, enviar_caracter);
}

void enviar_entero_serial(uint8_t numero) {
	enviar_entero(numero, enviar_caracter);
}

void mostrar_decimal_lcd(float numero) {
	enviar_decimal(numero, mostrar_caracter);
}

void mostrar_entero_lcd(uint8_t numero) {
	enviar_entero(numero, mostrar_caracter);
}

// Convertir el texto recibido por serial a un numero
float convertir_numero(const char *texto) {
	float resultado = 0.0f;
	float factor = 1.0f;
	uint8_t parte_decimal = 0;

	while (*texto) {
		if (*texto == '.') {
			parte_decimal = 1;
			} else if (*texto >= '0' && *texto <= '9') {
			if (!parte_decimal) {
				resultado = resultado * 10.0f + (*texto - '0');
				} else {
				factor /= 10.0f;
				resultado += (*texto - '0') * factor;
			}
		}
		texto++;
	}

	return resultado;
}

// -------------------- SENSOR DHT11 --------------------

static uint8_t esperar_nivel(uint8_t nivel, uint16_t tiempo_max) {
	uint16_t tiempo = 0;

	while (((DHT_PINR & (1 << DHT_BIT)) ? 1 : 0) != nivel) {
		if (++tiempo > tiempo_max) return 0;
		_delay_us(1);
	}

	return 1;
}

uint8_t leer_sensor(float *temp_leida, uint8_t *hum_leida) {
	uint8_t datos[5] = {0, 0, 0, 0, 0};
	uint8_t registro = SREG;

	cli();

	// Avisar al sensor que queremos leerlo
	DHT_DDR |= (1 << DHT_BIT);
	DHT_PORT &= ~(1 << DHT_BIT);
	SREG = registro;
	_delay_ms(20);
	cli();

	// Dejar el pin como entrada
	DHT_PORT |= (1 << DHT_BIT);
	_delay_us(30);
	DHT_DDR &= ~(1 << DHT_BIT);

	// Esperar la respuesta del sensor
	if (!esperar_nivel(0, 100)) goto error;
	if (!esperar_nivel(1, 100)) goto error;
	if (!esperar_nivel(0, 100)) goto error;

	// Leer los 40 bits enviados por el sensor
	for (uint8_t i = 0; i < 40; i++) {
		if (!esperar_nivel(1, 100)) goto error;
		_delay_us(40);

		if (DHT_PINR & (1 << DHT_BIT)) {
			datos[i / 8] |= (1 << (7 - (i % 8)));
		}

		if (!esperar_nivel(0, 100)) goto error;
	}

	SREG = registro;

	// Comprobar que los datos llegaron correctamente
	if ((uint8_t)(datos[0] + datos[1] + datos[2] + datos[3]) != datos[4]) {
		return 0;
	}

	*hum_leida = datos[0];

	float temp = (float)datos[2] + (float)(datos[3] & 0x7F) / 10.0f;
	if (datos[3] & 0x80) temp = -temp;
	*temp_leida = temp;

	return 1;

	error:
	SREG = registro;
	return 0;
}

// -------------------- TEMPORIZADORES Y PWM --------------------

void iniciar_pwm(void) {
	DDRB |= (1 << PB1) | (1 << PB0); // LED verde en pin 8 y rojo en pin 9
	TCCR1A = (1 << WGM10);
	TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10);
	OCR1A = 0;
}

void iniciar_temporizador(void) {
	TCCR0A = (1 << WGM01);
	TCCR0B = (1 << CS01) | (1 << CS00);
	OCR0A = 249;
	TIMSK0 |= (1 << OCIE0A);
}

ISR(TIMER0_COMPA_vect) {
	static uint16_t contador = 0;
	contador++;

	if (contador >= 5000) {
		tomar_muestra = 1;
		contador = 0;
	}
}

// -------------------- RECIBIR COMANDOS --------------------

char buffer_serial[16];
volatile uint8_t indice_serial = 0;
volatile uint8_t comando_listo = 0;

ISR(USART_RX_vect) {
	char caracter = UDR0;

	if (caracter == '\r' || caracter == '\n') {
		if (indice_serial > 0) {
			buffer_serial[indice_serial] = '\0';
			comando_listo = 1;
			indice_serial = 0;
		}
		} else if (indice_serial < sizeof(buffer_serial) - 1) {
		buffer_serial[indice_serial++] = caracter;
	}
}

void leer_comando(void) {
	if (!comando_listo) return;

	float nuevo_objetivo = convertir_numero(buffer_serial);

	if (nuevo_objetivo <= 5.0f ||
	(nuevo_objetivo + SAFETY_MARGIN) >= MAX_SENSOR_TEMP) {
		enviar_texto("\r\n[ERROR] Punto medio invalido!\r\n");
		} else {
		temperatura_objetivo = nuevo_objetivo;
		enviar_texto("\r\n[OK] Nuevo PM: ");
		enviar_decimal_serial(temperatura_objetivo);
		enviar_texto(" C\r\n");
	}

	comando_listo = 0;
}

// -------------------- CONTROL DE TEMPERATURA --------------------

void controlar_temperatura(float temp, uint8_t hum) {
	const char *accion = "";

	float limite_bajo = temperatura_objetivo - 4.5f;
	float limite_alto = temperatura_objetivo + 4.5f;

	float limite_ventilador_bajo = limite_alto;
	float limite_ventilador_medio = limite_alto + 3.0f;
	float limite_ventilador_alto = limite_alto + 6.0f;

	const float HIST = 0.2f;
	uint8_t cambio_estado;

	// Elegir el estado segun la temperatura medida
	do {
		cambio_estado = 0;

		switch (estado_actual) {
			case CALEFACTOR:
			if (temp >= (limite_bajo + HIST)) {
				estado_actual = IDEAL;
				cambio_estado = 1;
			}
			break;

			case IDEAL:
			if (temp <= (limite_bajo - HIST)) {
				estado_actual = CALEFACTOR;
				cambio_estado = 1;
				} else if (temp >= (limite_ventilador_bajo + HIST)) {
				estado_actual = VENTILADOR_BAJO;
				cambio_estado = 1;
			}
			break;

			case VENTILADOR_BAJO:
			if (temp <= (limite_ventilador_bajo - HIST)) {
				estado_actual = IDEAL;
				cambio_estado = 1;
				} else if (temp >= (limite_ventilador_medio + HIST)) {
				estado_actual = VENTILADOR_MEDIO;
				cambio_estado = 1;
			}
			break;

			case VENTILADOR_MEDIO:
			if (temp <= (limite_ventilador_medio - HIST)) {
				estado_actual = VENTILADOR_BAJO;
				cambio_estado = 1;
				} else if (temp >= (limite_ventilador_alto + HIST)) {
				estado_actual = VENTILADOR_ALTO;
				cambio_estado = 1;
			}
			break;

			case VENTILADOR_ALTO:
			if (temp <= (limite_ventilador_alto - HIST)) {
				estado_actual = VENTILADOR_MEDIO;
				cambio_estado = 1;
			}
			break;
		}
	} while (cambio_estado);

	// Encender los LED segun el estado actual
	switch (estado_actual) {
		case CALEFACTOR:
		TCCR1A &= ~(1 << COM1A1);
		PORTB &= ~(1 << PB1);
		PORTB |= (1 << PB0);
		accion = "Calefactor ON";
		break;

		case IDEAL:
		TCCR1A &= ~(1 << COM1A1);
		PORTB &= ~((1 << PB0) | (1 << PB1));
		accion = "Ideal (Todo OFF)";
		break;

		case VENTILADOR_BAJO:
		TCCR1A |= (1 << COM1A1);
		PORTB &= ~(1 << PB0);
		OCR1A = 25;
		accion = "Ventilador BAJO (10%)";
		break;

		case VENTILADOR_MEDIO:
		TCCR1A |= (1 << COM1A1);
		PORTB &= ~(1 << PB0);
		OCR1A = 128;
		accion = "Ventilador MEDIO (50%)";
		break;

		case VENTILADOR_ALTO:
		TCCR1A |= (1 << COM1A1);
		PORTB &= ~(1 << PB0);
		OCR1A = 255;
		accion = "Ventilador ALTO (100%)";
		break;
	}

	// Mostrar los datos por el monitor serial
	enviar_texto("TEMP:");
	enviar_decimal_serial(temp);
	enviar_texto("|HUM:");
	enviar_entero_serial(hum);
	enviar_texto("|ACCION:");
	enviar_texto(accion);
	enviar_texto("|PM:");
	enviar_decimal_serial(temperatura_objetivo);
	enviar_texto("\r\n");

	// Mostrar los datos en la pantalla LCD
	ubicar_cursor(0, 0);
	mostrar_texto("T:");
	mostrar_decimal_lcd(temp);
	mostrar_texto("C H:");
	mostrar_entero_lcd(hum);
	mostrar_texto("%  ");

	ubicar_cursor(1, 0);
	mostrar_texto("Rango:");
	mostrar_decimal_lcd(limite_bajo);
	mostrar_caracter('-');
	mostrar_decimal_lcd(limite_alto);
	mostrar_texto("  ");
}

// -------------------- PROGRAMA PRINCIPAL --------------------

int main(void) {
	iniciar_serial(9600);
	iniciar_lcd();
	iniciar_pwm();
	iniciar_temporizador();

	// Dejar el pin del DHT11 como entrada con pull-up
	DHT_DDR &= ~(1 << DHT_BIT);
	DHT_PORT |= (1 << DHT_BIT);

	sei();

	enviar_texto("\r\n=== SISTEMA CONTROL TEMPERATURA ATmega328P (DHT11) ===\r\n");

	_delay_ms(1000); // Esperar a que el sensor este listo
	tomar_muestra = 1;

	while (1) {
		leer_comando();

		if (tomar_muestra) {
			tomar_muestra = 0;

			if (leer_sensor(&temperatura, &humedad)) {
				controlar_temperatura(temperatura, humedad);
				} else {
				enviar_texto("[ERROR] Fallo de lectura DHT11\r\n");
				ubicar_cursor(0, 0);
				mostrar_texto("Error sensor    ");
			}
		}
	}
}


