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

#define LED         PB0
#define BUZZER      PB1
#define BOTON       PD2     // INT0


#define TIEMPO_LED_MS       200UL
#define TIEMPO_LCD_MS       500UL
#define TIEMPO_TEMP_MS      2000UL
#define TIEMPO_BUZZER_MS    300UL

//Funcion Puntero A funcion Estado
typedef void(*EstadoFunc)(void);

//Declaración de función de cada estado
void st_activo(void);
void st_standby(void);
void st_apagado(void);

//Variable global que guarda la dirección del estado actual
//'volatile' evita que el compilador optimice o descarte la variable si cambia en una ISR
volatile EstadoFunc estado_actual = st_activo;

volatile uint32_t tiempo_ms = 0;
volatile uint8_t evento_despertar = 0;

static uint32_t ultimo_led = 0;
static uint32_t ultima_actividad = 0;
static uint32_t inicio_buzzer = 0;
static bool buzzer_activo = false;

// Lectura segura del contador de 32 bits usando cli() y sei()
static uint32_t obtener_tiempo(void) {
	uint32_t copia;
	cli();             // Bloquea interrupciones para evitar lectura corrupta
	copia = tiempo_ms; // Copia segura de los 4 bytes
	sei();             // Vuelve a habilitar interrupciones
	return copia;
}

static void configurar_timer0(void) {
	TCCR0A = (1 << WGM01);
	TCCR0B = 0;
	TCNT0 = 0;
	OCR0A = 249;
	TIMSK0 = (1 << OCIE0A);
	TCCR0B = (1 << CS01) | (1 << CS00); // Prescaler 64 -> Genera interrupción cada 1 ms
}

static void configurar_int0(void) {
	DDRD &= ~(1 << BOTON);
	PORTD |= (1 << BOTON);
	EICRA = (1 << ISC01); // Interrupción por flanco descendente
	EIMSK = (1 << INT0);
}

static void configurar_usart(void) {
	UBRR0H = 0;
	UBRR0L = 103; // Configuración a 9600 baudios para reloj de 16 MHz
	UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0); // Habilita RX, TX e interrupción por recepción
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // Trama de 8 bits de datos, 1 bit de parada
}

static void inicializar_hardware(void) {
	cli();
	DDRB |= (1 << LED) | (1 << BUZZER);
	PORTB &= ~((1 << LED) | (1 << BUZZER));

	configurar_timer0();
	configurar_int0();
	configurar_usart();
	
	sei();
}

//Partes del codigo del Problema A

static void tarea_led(uint32_t ahora) {
	if ((uint32_t)(ahora - ultimo_led) >= TIEMPO_LED_MS) {
		ultimo_led = ahora;
		PORTB ^= (1 << LED);
	}
}

static void activar_buzzer(uint32_t ahora) {
	PORTB |= (1 << BUZZER);
	buzzer_activo = true;
	inicio_buzzer = ahora;
}

static void tarea_buzzer(uint32_t ahora) {
	if (buzzer_activo && (uint32_t)(ahora - inicio_buzzer) >= TIEMPO_BUZZER_MS) {
		PORTB &= ~(1 << BUZZER);
		buzzer_activo = false;
	}
}

void st_activo(void) {

     uint32_t ahora = obtener_tiempo();

      if (evento_despertar) {
	   evento_despertar = 0;
	   ultima_actividad = ahora;
	   activar_buzzer(ahora);
}

tarea_led(ahora);
tarea_buzzer(ahora);

if ((uint32_t)(ahora - ultima_actividad) >= TIEMPO_STANDBY_MS) {
	estado_actual = st_standby;
}
}

void st_standby(void) {
	uint32_t ahora = obtener_tiempo();

	if (evento_despertar) {
		estado_actual = st_activo;
		return;
	}

	PORTB |= (1 << LED); // LED encendido fijo
	tarea_buzzer(ahora);

	if ((uint32_t)(ahora - ultima_actividad) >= TIEMPO_APAGADO_MS) {
		estado_actual = st_apagado;
	}
}

void st_apagado(void) {
	uint32_t ahora = obtener_tiempo();

	PORTB &= ~((1 << LED) | (1 << BUZZER)); // Apaga todo

	if (evento_despertar) {
		estado_actual = st_activo;
		return;
	}

	tarea_buzzer(ahora);
}

// Interrupción de tiempo
ISR(TIMER0_COMPA_vect) {
	tiempo_ms++;
}

// ISR del Botón (INT0 en PD2 - Pin 32)
ISR(INT0_vect) {
	evento_despertar = 1;
	estado_actual = st_activo;
}

// ISR de Bluetooth (UART RX)
ISR(USART_RX_vect) {
	uint8_t dummy = UDR0;
	(void)dummy;
	evento_despertar = 1;
	estado_actual = st_activo;
}

int main(void)
{
	inicializar_hardware();
	ultima_actividad = obtener_tiempo();

	while (1) {
		estado_actual();
	}

	return 0;
}
   


