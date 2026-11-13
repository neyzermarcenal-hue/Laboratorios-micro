/*
 * Problema B - Lab5.c
 *
 * Created: 29/10/2026 17:39:43
 * Author : cvarg
 */ 

#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>  // Para los modos Sleep
#include <avr/wdt.h>    // Para el Watchdog
#include <stdint.h>
#include <stdbool.h>
#include <util/delay.h>
#include <util/atomic>
#include <avr/power.h>

#define LED         PB0
#define BUZZER      PB1
#define BOTON       PD2     // INT0
#define BOTON_EMERGENCIA PD3 
#define CANAL_ADC 0
#define DHT11_PIN PD4

#define LCD_DIRECCION 0x27

#define LCD_RS 0x01
#define LCD_EN 0x04
#define LCD_LUZ 0x08

#define LCD_D4 0x10
#define LCD_D5 0x20
#define LCD_D6 0x40
#define LCD_D7 0x80

#define BAUDRATE         9600
#define UBRR_VAL         ((F_CPU / (16UL * BAUDRATE)) - 1)
#define BUF_SIZE         32

#define TRAMA_START      ':'
#define TRAMA_STOP       '\n'

#define TIEMPO_LED_MS       200UL
#define TIEMPO_LCD_MS       500UL
#define TIEMPO_TEMP_MS      2000UL
#define TIEMPO_BUZZER_MS    300UL

#define TIEMPO_STANDBY_MS 10000UL
#define TIEMPO_APAGADO_MS 30000UL
#define REBOTE_BOTON_MS      50UL
#define TIEMPO_SALIDA_EMERGENCIA_MS 3000UL

static uint32_t inicio_presion_emergencia = 0;
static bool boton_emergencia_presionado = false;
static bool salida_emergencia_habilitada = false;
static uint8_t ultima_alarma = 0;
static uint8_t notificacion_pendiente = 0;

typedef enum
{
	ACTIVO = 0,
	STANDBY,
	APAGADO,
	EMERGENCIA,
	NUM_ESTADOS
} EstadoSistema;

typedef void (*HandlerEstado)(uint32_t ahora);

typedef struct
{
	EstadoSistema estado;
	HandlerEstado handler;
} TransicionFSM;

static EstadoSistema estado_actual = ACTIVO;

volatile uint32_t tiempo_ms = 0;

volatile uint8_t solicitar_adc = 0;
volatile uint8_t evento_boton = 0;
volatile uint8_t evento_emergencia = 0;
volatile uint8_t wdt_despertador_flag = 0;
volatile uint8_t causante_reset_wdt = 0;

volatile char rx_buffer[BUF_SIZE];
volatile uint8_t rx_head = 0;
volatile uint8_t rx_tail = 0;

static uint16_t valor_adc = 0;

static uint32_t ultimo_led = 0;
static uint32_t ultima_lcd = 0;
static uint32_t ultima_temp = 0;
static uint32_t ultima_actividad = 0;
static uint32_t ultimo_boton = 0;
static uint32_t inicio_buzzer = 0;

static bool buzzer_activo = false;
static bool primer_boton = true;

// Lectura segura del contador de 32 bits usando cli() y sei()
static uint32_t obtener_tiempo(void) {
	uint32_t copia;
	cli();             // Bloquea interrupciones para evitar lectura corrupta
	copia = tiempo_ms; // Copia segura de los 4 bytes
	sei();             // Vuelve a habilitar interrupciones
	return copia;
}

typedef enum
{
	DHT_REPOSO,
	DHT_INICIO,
	DHT_RESPUESTA,
	DHT_RECEPCION
} EstadoDHT;

static EstadoDHT estado_dht = DHT_REPOSO;

volatile uint8_t dht_datos[5] = {0};
volatile uint8_t dht_bits = 0;
volatile uint8_t dht_fase = 0;
volatile uint8_t dht_listo = 0;
volatile uint8_t dht_error = 0;

static uint8_t temperatura = 0;
static uint8_t humedad = 0;

static bool dht_valido = false;

static uint32_t dht_inicio_ms = 0;

// Declaraciones previas

static void lcd_retroiluminacion(uint8_t encendida);
static void activar_buzzer(uint32_t ahora);
static void restaurar_perifericos_despertar(void);
static void tarea_temperatura(uint32_t ahora);

//Interrupciones 

ISR(TIMER0_COMPA_vect)
{
	tiempo_ms++;
}

ISR(TIMER1_COMPA_vect)
{
	solicitar_adc = 1;
}

ISR(INT0_vect)
{
	evento_boton = 1;
}

ISR(INT1_vect)
{
	evento_emergencia = 1;
}

// Watchdog: interrupción periódica

ISR(WDT_vect)
{
	wdt_despertador_flag = 1;

	// El hardware deshabilita WDIE al entrar
	// en la interrupción. Lo habilitamos nuevamente.
	WDTCSR |= (1 << WDIE);
}

// USART RX

ISR(USART_RX_vect)
{
	char c = UDR0;

	uint8_t next = (rx_head + 1) % BUF_SIZE;

	if (next != rx_tail)
	{
		rx_buffer[rx_head] = c;
		rx_head = next;
	}
}

static void uart_init(void)
{
	UBRR0H = (uint8_t)(UBRR_VAL >> 8);
	UBRR0L = (uint8_t)UBRR_VAL;

	UCSR0B = (1 << RXEN0) |
	(1 << TXEN0) |
	(1 << RXCIE0);

	UCSR0C = (1 << UCSZ01) |
	(1 << UCSZ00);
}

static void uart_transmit(char data)
{
	while (!(UCSR0A & (1 << UDRE0)));

	UDR0 = data;
}

static void uart_send_string(const char *str)
{
	while (*str)
	{
		uart_transmit(*str++);
	}
}

static bool uart_read_char(char *c)
{
	if (rx_head == rx_tail)
	return false;

	*c = rx_buffer[rx_tail];

	rx_tail = (rx_tail + 1) % BUF_SIZE;

	return true;
}

static void uart_enviar_trama(char cmd, uint8_t dato)
{
	uint8_t chk = (uint8_t)cmd ^ dato;

	uart_transmit(TRAMA_START);
	uart_transmit(cmd);
	uart_transmit(dato);
	uart_transmit(chk);
	uart_transmit(TRAMA_STOP);
}

static void procesar_bluetooth(uint32_t ahora)
{
	static char buffer_trama[5];
	static uint8_t idx = 0;
	char c;

	while (uart_read_char(&c))
	{
		if (idx == 0 && c != TRAMA_START)
		{
			continue;
		}

		buffer_trama[idx++] = c;

		if (idx == 5)
		{
			if (buffer_trama[0] == TRAMA_START &&
			buffer_trama[4] == TRAMA_STOP)
			{
				char cmd = buffer_trama[1];
				uint8_t dato = (uint8_t)buffer_trama[2];
				uint8_t chk = (uint8_t)buffer_trama[3];

				if (((uint8_t)cmd ^ dato) == chk)
				{
					// Consultar estado
					if (cmd == 'S')
					{
						uart_enviar_trama('S',
						(uint8_t)estado_actual);
					}

					// Consultar temperatura
					else if (cmd == 'T')
					{
						uart_enviar_trama('T', temperatura);
					}

					// Consultar ultima alarma
					else if (cmd == 'L')
					{
						uart_enviar_trama('L', ultima_alarma);
					}

					// Activar buzzer
					else if (cmd == 'A')
					{
						if (estado_actual == EMERGENCIA)
						{
							uart_enviar_trama('A', 0);
						}
						else
						{
							ultima_actividad = ahora;

							if (estado_actual == STANDBY)
							{
								estado_actual = ACTIVO;
								lcd_retroiluminacion(1);
							}

							activar_buzzer(ahora);
							uart_enviar_trama('A', 1);
						}
					}

					// Activar emergencia
					else if (cmd == 'E')
					{
						ultima_actividad = ahora;

						if (estado_actual == STANDBY)
						{
							restaurar_perifericos_despertar();
							lcd_retroiluminacion(1);
						}

						if (estado_actual != EMERGENCIA)
						{
							evento_emergencia = 1;
						}

						uart_enviar_trama('E', 1);
					}

					// Volver a ACTIVO
					else if (cmd == 'C')
					{
						if (estado_actual == EMERGENCIA)
						{
							// No permite anular la emergencia
							uart_enviar_trama('C', 3);
						}
						else
						{
							ultima_actividad = ahora;
							
							if (estado_actual == STANDBY)
							{
								restaurar_perifericos_despertar();
								lcd_retroiluminacion(1);
							}

							estado_actual = ACTIVO;
							uart_enviar_trama('C', ACTIVO);
						}
					}
				}
			}

			idx = 0;
		}
	}
}

//WDT

void capturar_mcusr(void)
_attribute_((naked))
_attribute_((section(".init3")));

void capturar_mcusr(void)
{
	causante_reset_wdt = MCUSR;

	MCUSR = 0;

	wdt_disable();
}

static void configurar_wdt_interrupcion(void)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		wdt_reset();

		WDTCSR |= (1 << WDCE) | (1 << WDE);

		WDTCSR = (1 << WDIE) |
		(1 << WDP3) |
		(1 << WDP0);
	}
}

static uint32_t obtener_tiempo(void)
{
	uint32_t copia;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		copia = tiempo_ms;
	}

	return copia;
}

static void configurar_timer0(void)
{
	TCCR0A = (1 << WGM01);

	TCCR0B = 0;

	TCNT0 = 0;

	OCR0A = 249;

	TIFR0 = (1 << OCF0A);

	TIMSK0 = (1 << OCIE0A);

	TCCR0B = (1 << CS01) | (1 << CS00);
}

static void configurar_timer1(void)
{
	TCCR1A = 0;

	TCCR1B = 0;

	TCNT1 = 0;

	OCR1A = 24999;

	TIFR1 = (1 << OCF1A);

	TIMSK1 = (1 << OCIE1A);

	TCCR1B = (1 << WGM12) |
	(1 << CS11) |
	(1 << CS10);
}

static void configurar_adc(void)
{
	ADMUX = (1 << REFS0) | CANAL_ADC;

	ADCSRA = (1 << ADEN) |
	(1 << ADPS2) |
	(1 << ADPS1) |
	(1 << ADPS0);

	ADCSRB = 0;

	DIDR0 |= (1 << ADC0D);
}

static void configurar_int0(void)
{
	DDRD &= ~(1 << BOTON);

	PORTD |= (1 << BOTON);

	EICRA |= (1 << ISC01);

	EIFR = (1 << INTF0);

	EIMSK |= (1 << INT0);
}

static void configurar_int1(void)
{
	DDRD &= ~(1 << BOTON_EMERGENCIA);

	PORTD |= (1 << BOTON_EMERGENCIA);

	EICRA |= (1 << ISC11);

	EIFR = (1 << INTF1);

	EIMSK |= (1 << INT1);
}

static uint8_t lcd_luz = LCD_LUZ;

static void i2c_init(void)
{
	TWSR = 0;

	TWBR = 72;

	TWCR = (1 << TWEN);
}

static uint8_t i2c_esperar(void)
{
	uint16_t intentos = 60000;

	while (!(TWCR & (1 << TWINT)))
	{
		if (--intentos == 0)
		return 0;
	}

	return 1;
}

static void i2c_detener(void)
{
	TWCR = (1 << TWINT) |
	(1 << TWEN) |
	(1 << TWSTO);
}

static uint8_t i2c_escribir(uint8_t dato)
{
	TWCR = (1 << TWINT) |
	(1 << TWSTA) |
	(1 << TWEN);

	if (!i2c_esperar())
	{
		i2c_detener();
		return 0;
	}

	if ((TWSR & 0xF8) != 0x08 &&
	(TWSR & 0xF8) != 0x10)
	{
		i2c_detener();
		return 0;
	}

	TWDR = LCD_DIRECCION << 1;

	TWCR = (1 << TWINT) | (1 << TWEN);

	if (!i2c_esperar())
	{
		i2c_detener();
		return 0;
	}

	if ((TWSR & 0xF8) != 0x18)
	{
		i2c_detener();
		return 0;
	}

	TWDR = dato;

	TWCR = (1 << TWINT) | (1 << TWEN);

	if (!i2c_esperar())
	{
		i2c_detener();
		return 0;
	}

	uint8_t correcto = ((TWSR & 0xF8) == 0x28);

	i2c_detener();
	
		return correcto;
	}
	
static void lcd_pulso(uint8_t dato)
{
	i2c_escribir(dato | LCD_EN);

	_delay_us(1);

	i2c_escribir(dato & ~LCD_EN);

	_delay_us(50);
}

static void lcd_nibble(uint8_t nibble, uint8_t rs)
{
	uint8_t dato = (nibble & 0xF0) | lcd_luz;

	if (rs)
	dato |= LCD_RS;

	lcd_pulso(dato);
}

static void lcd_enviar(uint8_t valor, uint8_t rs)
{
	lcd_nibble(valor & 0xF0, rs);

	lcd_nibble((valor << 4) & 0xF0, rs);
}

static void lcd_comando(uint8_t comando)
{
	lcd_enviar(comando, 0);

	if (comando == 0x01 || comando == 0x02)
	_delay_ms(2);
}

static void lcd_caracter(char caracter)
{
	lcd_enviar((uint8_t)caracter, 1);
}

static void lcd_texto(const char *texto)
{
	while (*texto)
	{
		lcd_caracter(*texto++);
	}
}

static void lcd_posicion(uint8_t fila, uint8_t columna)
{
	uint8_t direccion = (fila == 0) ? 0x00 : 0x40;

	lcd_comando(0x80 | (direccion + columna));
}

// Escribe un numero ocupando siempre el mismo ancho.
// Ejemplo: 1000 -> "1000"; 999 -> " 999".

static void lcd_numero_ancho(uint16_t numero, uint8_t ancho)
{
	uint16_t divisor = 1;

	for (uint8_t i = 1; i < ancho; i++)
	{
		divisor *= 10;
	}

	for (uint8_t i = 0; i < ancho; i++)
	{
		uint8_t digito = numero / divisor;

		lcd_caracter('0' + digito);

		numero %= divisor;

		divisor /= 10;
	}
}

static void lcd_init(void)
{
	_delay_ms(50);

	lcd_nibble(0x30, 0);

	_delay_ms(5);

	lcd_nibble(0x30, 0);

	_delay_us(150);

	lcd_nibble(0x30, 0);

	lcd_nibble(0x20, 0);

	lcd_comando(0x28);

	lcd_comando(0x0C);

	lcd_comando(0x06);

	lcd_comando(0x01);
}

static void lcd_retroiluminacion(uint8_t encendida)
{
	lcd_luz = encendida ? LCD_LUZ : 0;

	i2c_escribir(lcd_luz);
}

#define DHT_ESPERANDO_BAJADA  0
#define DHT_ESPERANDO_SUBIDA  1
#define DHT_FIN_RESPUESTA     2
#define DHT_INICIO_BIT        3
#define DHT_FIN_BIT           4

static void configurar_dht11(void)
{
	TCCR2A = 0;

	TCCR2B = (1 << CS21);

	TCNT2 = 0;

	DDRD &= ~(1 << DHT11_PIN);

	PORTD |= (1 << DHT11_PIN);

	PCMSK2 |= (1 << PCINT20);

	PCICR &= ~(1 << PCIE2);

	PCIFR = (1 << PCIF2);
}

static void dht_finalizar(uint8_t error)
{
	PCICR &= ~(1 << PCIE2);

	if (error)
	dht_error = 1;
	else
	dht_listo = 1;
}

ISR(PCINT2_vect)
{
	uint8_t nivel = PIND & (1 << DHT11_PIN);

	uint8_t fase = dht_fase;

	if (fase == DHT_ESPERANDO_BAJADA)
	{
		if (!nivel)
		dht_fase = DHT_ESPERANDO_SUBIDA;
	}
	else if (fase == DHT_ESPERANDO_SUBIDA)
	{
		if (nivel)
		dht_fase = DHT_FIN_RESPUESTA;
	}
	else if (fase == DHT_FIN_RESPUESTA)
	{
		if (!nivel)
		dht_fase = DHT_INICIO_BIT;
	}
	else if (fase == DHT_INICIO_BIT)
	{
		if (nivel)
		{
			TCNT2 = 0;

			dht_fase = DHT_FIN_BIT;
		}
	}
	else if (fase == DHT_FIN_BIT)
	{
		if (!nivel)
		{
			uint8_t bit = (TCNT2 > 100) ? 1 : 0;

			uint8_t indice = dht_bits / 8;

			dht_datos[indice] =
			(dht_datos[indice] << 1) | bit;

			dht_bits++;

			if (dht_bits >= 40)
			{
				dht_finalizar(0);
			}
			else
			{
				dht_fase = DHT_INICIO_BIT;
			}
		}
	}
}

static void inicializar_hardware(void)
{
	cli();

	DDRB |= (1 << LED) | (1 << BUZZER);

	PORTB &= ~((1 << LED) | (1 << BUZZER));

	configurar_timer0();

	configurar_timer1();

	configurar_adc();

	configurar_int0();

	configurar_int1();

	configurar_dht11();

	uart_init();

	i2c_init();

	configurar_wdt_interrupcion();

	sei();

	lcd_init();

	if (causante_reset_wdt & (1 << WDRF))
	{
		uart_send_string(
		"[ALERTA] Sistema reiniciado por WDT Reset!\r\n"
		);
	}
}
	
static void tarea_led(uint32_t ahora)
{
	if ((uint32_t)(ahora - ultimo_led) >= TIEMPO_LED_MS)
	{
		ultimo_led = ahora;

		PORTB ^= (1 << LED);
	}
}

static void tarea_adc(void)
{
	if (solicitar_adc == 0)
	return;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		solicitar_adc = 0;
	}

	if (!(ADCSRA & (1 << ADSC)))
	{
		ADCSRA |= (1 << ADSC);
	}
}

static void comprobar_adc(void)
{
	if (ADCSRA & (1 << ADSC))
	return;

	if (ADCSRA & (1 << ADIF))
	{
		valor_adc = ADC;

		ADCSRA |= (1 << ADIF);
	}
}

static void activar_buzzer(uint32_t ahora)
{
	PORTB |= (1 << BUZZER);

	buzzer_activo = true;

	inicio_buzzer = ahora;
}

static void tarea_buzzer(uint32_t ahora)
{
	if (buzzer_activo &&
	(uint32_t)(ahora - inicio_buzzer) >= TIEMPO_BUZZER_MS)
	{
		PORTB &= ~(1 << BUZZER);

		buzzer_activo = false;
	}
}

static void atender_boton(uint32_t ahora)
{
	uint8_t pendiente;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		pendiente = evento_boton;

		evento_boton = 0;
	}

	if (!pendiente)
	return;

	if (!primer_boton &&
	(uint32_t)(ahora - ultimo_boton) < REBOTE_BOTON_MS)
	{
		return;
	}

	primer_boton = false;

	ultimo_boton = ahora;

	ultima_actividad = ahora;

	if (estado_actual == STANDBY ||
	estado_actual == APAGADO)
	{
		restaurar_perifericos_despertar();

		estado_actual = ACTIVO;

		lcd_retroiluminacion(1);
	}

	if (estado_actual != EMERGENCIA)
	{
		activar_buzzer(ahora);
	}
}

static void atender_emergencia(void)
{
	uint8_t pendiente;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		pendiente = evento_emergencia;
		evento_emergencia = 0;
	}

	// Activar emergencia
	if (pendiente && estado_actual != EMERGENCIA)
	{
		estado_actual = EMERGENCIA;

		PORTB &= ~((1 << LED) | (1 << BUZZER));
		buzzer_activo = false;

		boton_emergencia_presionado = false;
		salida_emergencia_habilitada = false;

		ultima_alarma = 1;
		notificacion_pendiente = 1;
	}

	if (estado_actual == EMERGENCIA)
	{
		if (!(PIND & (1 << BOTON_EMERGENCIA)))
		{
			if (!boton_emergencia_presionado)
			{
				boton_emergencia_presionado = true;
				inicio_presion_emergencia = obtener_tiempo();
			}

			uint32_t ahora = obtener_tiempo();

			if ((uint32_t)(ahora -
			inicio_presion_emergencia) >=
			TIEMPO_SALIDA_EMERGENCIA_MS)
			{
				salida_emergencia_habilitada = true;
			}
		}
		else
		{
			if (boton_emergencia_presionado && salida_emergencia_habilitada)
		
		
				{
					estado_actual = ACTIVO;

					ultima_actividad = obtener_tiempo();
					ultimo_led = obtener_tiempo();

					PORTB &= ~((1 << LED) | (1 << BUZZER));
					buzzer_activo = false;

					lcd_retroiluminacion(1);

					ultima_alarma = 2;
					notificacion_pendiente = 2;
				}

				boton_emergencia_presionado = false;
				salida_emergencia_habilitada = false;
			}
		}
	}

	static void tarea_lcd(uint32_t ahora)
	{
		// Esperar a que el DHT11 termine su lectura.
		if (estado_dht != DHT_REPOSO)
		return;

		if ((uint32_t)(ahora - ultima_lcd) < TIEMPO_LCD_MS)
		return;

		ultima_lcd = ahora;

		// Primera fila: estado del sistema.

		lcd_posicion(0, 0);

		if (estado_actual == ACTIVO)
		{
			lcd_texto("ESTADO: ACTIVO  ");
		}
		else if (estado_actual == STANDBY)
		{
			lcd_texto("ESTADO: STANDBY ");
		}
		else if (estado_actual == APAGADO)
		{
			lcd_texto("ESTADO: APAGADO ");
		}
		else if (estado_actual == EMERGENCIA)
		{
			lcd_texto("EMERGENCIA      ");
		}

		// Segunda fila.

		lcd_posicion(1, 0);

		if (estado_actual == ACTIVO)
		{
			// Formato: A:1000 T:25 H:60
			// Siempre ocupa 16 caracteres.

			lcd_texto("A:");

			lcd_numero_ancho(valor_adc, 4);

			if (dht_valido)
			{
				lcd_texto(" T:");

				lcd_numero_ancho(temperatura, 2);

				lcd_texto(" H:");

				lcd_numero_ancho(humedad, 2);
			}
			else
			{
				lcd_texto(" DHT:--   ");
			}
		}
		else if (estado_actual == STANDBY)
		{
			lcd_texto("Esperando...    ");
		}
		else if (estado_actual == APAGADO)
		{
			lcd_texto("                ");
		}
		else if (estado_actual == EMERGENCIA)
		{
			lcd_texto("SISTEMA SEGURO  ");
		}
	}
	
	static void tarea_temperatura(uint32_t ahora)
	{
		if (estado_dht == DHT_REPOSO)
		{
			if ((uint32_t)(ahora - ultima_temp) < TIEMPO_TEMP_MS)
			return;

			ultima_temp = ahora;

			PCICR &= ~(1 << PCIE2);

			PORTD &= ~(1 << DHT11_PIN);

			DDRD |= (1 << DHT11_PIN);

			dht_inicio_ms = ahora;

			estado_dht = DHT_INICIO;

			return;
		}

		if (estado_dht == DHT_INICIO)
		{
			if ((uint32_t)(ahora - dht_inicio_ms) < 18UL)
			return;

			ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
			{
				dht_bits = 0;

				dht_fase = DHT_ESPERANDO_BAJADA;

				dht_listo = 0;

				dht_error = 0;

				for (uint8_t i = 0; i < 5; i++)
				{
					dht_datos[i] = 0;
				}

				PORTD |= (1 << DHT11_PIN);

				DDRD &= ~(1 << DHT11_PIN);

				TCNT2 = 0;

				PCIFR = (1 << PCIF2);

				PCICR |= (1 << PCIE2);
			}

			dht_inicio_ms = ahora;

			estado_dht = DHT_RECEPCION;

			return;
		}

		if (estado_dht == DHT_RECEPCION)
		{
			uint8_t datos[5];

			uint8_t listo;

			uint8_t error;

			ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
			{
				listo = dht_listo;

				error = dht_error;

				for (uint8_t i = 0; i < 5; i++)
				{
					datos[i] = dht_datos[i];
				}

				if (listo)
				dht_listo = 0;
			}

			if (listo)
			{
				if ((datos[0] + datos[1] +
				datos[2] + datos[3]) == datos[4])
				{
					humedad = datos[0];

					temperatura = datos[2];


estado_dht = DHT_REPOSO;

return;
}

if (error ||
(uint32_t)(ahora - dht_inicio_ms) >= 8UL)
{
	PCICR &= ~(1 << PCIE2);

	dht_valido = false;

	estado_dht = DHT_REPOSO;
}
}
}

static void restaurar_perifericos_despertar(void)
{
	power_all_enable();

	configurar_adc();

	configurar_timer1();

	configurar_dht11();

	uart_init();

	i2c_init();
}

static void entrar_modo_idle(void)
{
	set_sleep_mode(SLEEP_MODE_IDLE);

	sleep_enable();

	sleep_cpu();

	sleep_disable();
}

static void entrar_modo_power_down(void)
{
	lcd_retroiluminacion(0);

	ADCSRA &= ~(1 << ADEN);

	power_adc_disable();

	power_usart0_disable();

	power_spi_disable();

	power_timer1_disable();

	power_timer2_disable();

	power_twi_disable();

	set_sleep_mode(SLEEP_MODE_PWR_DOWN);

	sleep_enable();

	sleep_bod_disable();

	sei();

	sleep_cpu();

	sleep_disable();

	restaurar_perifericos_despertar();
}

static void handler_activo(uint32_t ahora)
{
	tarea_led(ahora);

	tarea_adc();

	comprobar_adc();

	tarea_temperatura(ahora);

	if ((uint32_t)(ahora - ultima_actividad) >=
	TIEMPO_STANDBY_MS)
	{
		estado_actual = STANDBY;
	}
}

static void handler_standby(uint32_t ahora)
{
	// Si el DHT11 ya estaba realizando una lectura,
	// permitir que termine.
	// No iniciar nuevas lecturas durante STANDBY.

	if (estado_dht != DHT_REPOSO)
	{
		tarea_temperatura(ahora);
	}

	if ((uint32_t)(ahora - ultima_actividad) >=
	TIEMPO_APAGADO_MS)
	{
		estado_actual = APAGADO;
	}
}

static void handler_apagado(uint32_t ahora)
{
	PORTB &= ~(1 << LED);

	entrar_modo_power_down();

	if (wdt_despertador_flag)
	{
		wdt_despertador_flag = 0;

		tarea_adc();

		comprobar_adc();
	}
}

static void handler_emergencia(uint32_t ahora)
{
	// Si había una lectura pendiente del DHT11,
	// permitir que termine.

	if (estado_dht != DHT_REPOSO)
	{
		tarea_temperatura(ahora);
	}

	PORTB &= ~((1 << LED) | (1 << BUZZER));

	buzzer_activo = false;
}

static const TransicionFSM gestor_estados[NUM_ESTADOS] =
{
	{ ACTIVO,     handler_activo },
	{ STANDBY,    handler_standby },
	{ APAGADO,    handler_apagado },
	{ EMERGENCIA, handler_emergencia }
};

static void actualizar_fsm(uint32_t ahora)
{
	if (estado_actual < NUM_ESTADOS)
	{
		gestor_estados[estado_actual].handler(ahora);
	}
}

int main(void)
{
	inicializar_hardware();

	uint32_t ahora = obtener_tiempo();

	ultimo_led = ahora;

	ultima_lcd = ahora;

	ultima_temp = ahora;

	ultima_actividad = ahora;

	while (1)
	{
		wdt_reset();

		ahora = obtener_tiempo();

		atender_boton(ahora);

		atender_emergencia();
		
		// Enviar notificaciones pendientes
		if (notificacion_pendiente != 0)
		{
			uint8_t evento = notificacion_pendiente;
			notificacion_pendiente = 0;

			uart_enviar_trama('N', evento);
		}

		procesar_bluetooth(ahora);

		actualizar_fsm(ahora);

		tarea_buzzer(ahora);

		tarea_lcd(ahora);
	}

	return 0;
}


