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
#define CANAL_ADC   0       // PC0 / ADC0

#define TIEMPO_LED_MS       200UL
#define TIEMPO_LCD_MS       500UL
#define TIEMPO_TEMP_MS      2000UL
#define TIEMPO_BUZZER_MS    300UL

#define TIEMPO_STANDBY_MS   10000UL
#define TIEMPO_APAGADO_MS   30000UL
#define REBOTE_BOTON_MS     50UL

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
volatile uint8_t solicitar_adc = 0;
volatile uint8_t evento_boton = 0;

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

static void configurar_timer0(void) {
	TCCR0A = (1 << WGM01);
	TCCR0B = 0;
	TCNT0 = 0;
	OCR0A = 249;
	TIFR0 = (1 << OCF0A);
	TIMSK0 = (1 << OCIE0A);
	TCCR0B = (1 << CS01) | (1 << CS00); // Prescaler 64 -> Genera interrupción cada 1 ms
}

static void configurar_timer1(void) {
	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1 = 0;
	OCR1A = 24999;
	TIFR1 = (1 << OCF1A);
	TIMSK1 = (1 << OCIE1A);
	TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10); // Prescaler 64 -> Genera disparo cada 100 ms
}

static void configurar_adc(void) {
	ADMUX = (1 << REFS0) | CANAL_ADC;
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
	ADCSRB = 0;
	DIDR0 |= (1 << ADC0D);
}

static void configurar_int0(void) {
	DDRD &= ~(1 << BOTON);
	PORTD |= (1 << BOTON);
	EICRA = (1 << ISC01); // Interrupción por flanco descendente
	EIFR = (1 << INTF0);
	EIMSK = (1 << INT0);
}

static void configurar_usart_bluetooth(void) {
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
	configurar_timer1();
	configurar_adc();
	configurar_int0();
	configurar_usart_bluetooth();
	
	wdt_enable(WDTO_2S); // Activa el Watchdog Timer de seguridad inicial a 2 segundos
	sei();
}

//Partes del codigo del Problema A

static void tarea_led(uint32_t ahora) {
	if ((uint32_t)(ahora - ultimo_led) >= TIEMPO_LED_MS) {
		ultimo_led = ahora;
		PORTB ^= (1 << LED);
	}
}

static void tarea_adc(void) {
	if (solicitar_adc == 0) return;

	cli();
	solicitar_adc = 0;
	sei();

	if (!(ADCSRA & (1 << ADSC))) ADCSRA |= (1 << ADSC);
}

static void comprobar_adc(void) {
	if (ADCSRA & (1 << ADSC)) return;
	if (ADCSRA & (1 << ADIF)) {
		valor_adc = ADC;
		ADCSRA |= (1 << ADIF);
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

static void atender_boton(uint32_t ahora) {
	uint8_t pendiente;

	cli();
	pendiente = evento_boton;
	evento_boton = 0;
	sei();

	if (!pendiente) return;

	if (!primer_boton && (uint32_t)(ahora - ultimo_boton) < REBOTE_BOTON_MS) return;

	primer_boton = false;
	ultimo_boton = ahora;
	ultima_actividad = ahora; // Reinicia el temporizador de inactividad del sistema

	if (estado_actual != st_activo) {
		estado_actual = st_activo; // Despierta el sistema al estado ACTIVO
	}
	activar_buzzer(ahora);
}
void st_activo(void) {
	// Reset de hardware del Watchdog Timer (evita que el perro muerda y reinicie el chip)
	wdt_reset();
	
    uint32_t ahora = obtener_tiempo();
	
	// Ejecución continua de tareas
	atender_boton(ahora);
	tarea_buzzer(ahora);
	tarea_led(ahora);
	tarea_adc();
	comprobar_adc();
	
// Evalúa transición a STANDBY tras 10 segundos de inactividad
if ((uint32_t)(ahora - ultima_actividad) >= TIEMPO_STANDBY_MS) {
	estado_actual = st_standby;
  }
}

void st_standby(void) {
	uint32_t ahora = obtener_tiempo();
	atender_boton(ahora);
	
	// Configurar SMCR para IDLE: Limpiar SM2, SM1, SM0 y activar SE (bit 0)
	SMCR = (1 << SE); // SM2:SM0 = 000 (Idle) y SE = 1
	// Ejecutar instrucción sleep 
	asm volatile("sleep");
	// Inmediatamente al despertar de una interrupción, deshabilitar SE por seguridad
	SMCR &= ~(1 << SE);

    // Evalúa transición a APAGADO tras 30 segundos de inactividad
    if ((uint32_t)(ahora - ultima_actividad) >= TIEMPO_APAGADO_MS) {
	estado_actual = st_apagado;
   }
}

void st_apagado(void) {
	
	PORTB &= ~(1 << LED); // Apaga el LED en el estado de reposo profundo
	//Corte de energía por hardware en el registro PRR
	// Desactiva ADC, Timer1, Timer2 y SPI
	PRR |= (1 << PRADC) | (1 << PRTIM1) | (1 << PRSPI);

	//Configurar WDT para MODO INTERRUPCIÓN a 8 segundos
	cli();                  // Deshabilitar interrupciones para secuencia crítica
	wdt_reset();           // Resetear contador interno
	MCUSR &= ~(1 << WDRF);  // Limpiar bandera de reset por WDT
	
	// Secuencia de cambio obligatoria: escribir 1 en WDCE y WDE al mismo tiempo
	WDTCSR |= (1 << WDCE) | (1 << WDE);
	
	// Escribir la nueva configuración:
	// WDIE = 1 (Generar interrupción, no reset), WDP3=1 y WDP0=1 (8 segundos de timeout)
	WDTCSR = (1 << WDIE) | (1 << WDP3) | (1 << WDP0);
	sei();                  // Recomenzar interrupciones

	//Configurar SMCR para POWER-DOWN (SM1 = 1, SM0 = 0, SM2 = 0) y SE = 1
	SMCR = (1 << SM1) | (1 << SE);

	//Dormir la CPU
	asm volatile("sleep");

	//Al despertar por la interrupción de 8s del WDT:
	SMCR &= ~(1 << SE); // Deshabilitar Sleep

	//Restaurar periféricos en el PRR al salir del estado
	PRR = 0x00;
}

//Rutinas de Interrupcion
ISR(TIMER0_COMPA_vect) { tiempo_ms++; }
ISR(TIMER1_COMPA_vect) { solicitar_adc = 1; }
ISR(INT0_vect)          { evento_boton = 1; }
ISR(WDT_vect)           { /* Despertador periódico en Power-Down */ }

// ISR de Recepción Serie (UART RX)
ISR(USART_RX_vect) {
	// Leer el byte del registro de datos de la USART (limpia la interrupción automáticamente)
	uint8_t rx_byte = UDR0;
	(void)dato_rx;

	ultima_actividad = obtener_tiempo(); // Reinicia inactividad y despierta
	estado_actual = st_activo;
}

int main(void)
{
	inicializar_hardware();
	ultima_actividad = obtener_tiempo();

    while (1) 
    {
		// Ejecución directa de la dirección de memoria de la función actual
		estado_actual();
	}

	return 0;
}
   


