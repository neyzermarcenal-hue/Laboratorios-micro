
#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <util/atomic.h>
#include <stdint.h>

// Configuracion de funcionamiento
// 0 = ACTIVO
// 1 = STANDBY
#define MODO_IDLE 0

volatile uint32_t tiempo_ms = 0;
volatile uint8_t pedir_adc = 0;
volatile uint8_t evento_boton = 0;
volatile uint16_t valor_adc = 0;

// Interrupciones

ISR(TIMER0_COMPA_vect)
{
	tiempo_ms++;
}

ISR(TIMER1_COMPA_vect)
{
	pedir_adc = 1;
}

ISR(ADC_vect)
{
	valor_adc = ADC;
}

ISR(PCINT2_vect)
{
	if (!(PIND & (1 << PD2)))
	{
		evento_boton = 1;
	}
}

// Lectura del tiempo

uint32_t leer_tiempo(void)
{
	uint32_t copia;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		copia = tiempo_ms;
	}

	return copia;
}

// Programa principal

int main(void)
{
	cli();

	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	// Configuracion de pines
	DDRB |= (1 << PB0) | (1 << PB1);
	PORTB &= ~((1 << PB0) | (1 << PB1));

	#if MODO_IDLE
	PORTB |= (1 << PB1);
	#endif

	DDRD &= ~(1 << PD2);
	PORTD |= (1 << PD2);

	PCMSK2 |= (1 << PCINT18);
	PCIFR = (1 << PCIF2);
	PCICR |= (1 << PCIE2);

	// Timer0 - Base de 1 ms
	TCCR0A = (1 << WGM01);
	TCCR0B = 0;
	TCNT0 = 0;
	OCR0A = 249;
	TIFR0 = (1 << OCF0A);
	TIMSK0 = (1 << OCIE0A);
	TCCR0B = (1 << CS01) | (1 << CS00);

	// Timer1 - Periodo de 100 ms
	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1 = 0;
	OCR1A = 24999;
	TIFR1 = (1 << OCF1A);
	TIMSK1 = (1 << OCIE1A);

	TCCR1B = (1 << WGM12) |
	(1 << CS11) |
	(1 << CS10);

	// ADC - Entrada A0
	ADMUX = (1 << REFS0);
	ADCSRB = 0;

	ADCSRA = (1 << ADEN) |
	(1 << ADIE) |
	(1 << ADPS2) |
	(1 << ADPS1) |
	(1 << ADPS0);

	DIDR0 |= (1 << ADC0D);

	sei();

	uint32_t ultimo_led = 0;

	// Planificacion de tareas

	while (1)
	{
		uint32_t ahora = leer_tiempo();

		if ((uint32_t)(ahora - ultimo_led) >= 200UL)
		{
			ultimo_led += 200UL;
			PORTB ^= (1 << PB0);
		}

		uint8_t solicitud;

		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			solicitud = pedir_adc;
			pedir_adc = 0;
		}

		if (solicitud && !(ADCSRA & (1 << ADSC)))
		{
			ADCSRA |= (1 << ADSC);
		}

		if (evento_boton)
		{
			evento_boton = 0;
		}

		// Modo STANDBY - Idle

		#if MODO_IDLE

		set_sleep_mode(SLEEP_MODE_IDLE);

		cli();

		if (!pedir_adc &&
		!evento_boton &&
		(uint32_t)(tiempo_ms - ultimo_led) < 200UL)
		{
			sleep_enable();
			sei();

			sleep_cpu();
			sleep_disable();
		}
		else
		{
			sei();
		}

		#endif
	}
}
