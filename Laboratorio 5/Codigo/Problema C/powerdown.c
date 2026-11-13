
#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <stdint.h>

// Configuracion de funcionamiento
// 1 = Power-Down con WDT
// 0 = Power-Down sin WDT
#define USAR_WDT 1

// 1 = Pulsos de diagnostico en D9
// 0 = Medicion de consumo
#define DIAGNOSTICO 0

volatile uint8_t evento_wdt = 0;
volatile uint8_t evento_boton = 0;
volatile uint16_t valor_adc = 0;

// Interrupciones

ISR(WDT_vect)
{
	evento_wdt = 1;
}

ISR(PCINT2_vect)
{
	if (!(PIND & (1 << PD2)))
	{
		evento_boton = 1;
	}
}

// Configuracion del Watchdog Timer

void iniciar_wdt(void)
{
	cli();

	wdt_reset();
	MCUSR &= ~(1 << WDRF);

	WDTCSR = (1 << WDCE) | (1 << WDE);

	WDTCSR = (1 << WDIE) |
	(1 << WDP3) |
	(1 << WDP0);

	sei();
}

// Lectura ADC al despertar

void leer_adc(void)
{
	PRR &= ~(1 << PRADC);

	ADMUX = (1 << REFS0);

	ADCSRA = (1 << ADEN) |
	(1 << ADPS2) |
	(1 << ADPS1) |
	(1 << ADPS0);

	_delay_ms(1);

	ADCSRA |= (1 << ADSC);

	while (ADCSRA & (1 << ADSC))
	{
	}

	valor_adc = ADC;

	ADCSRA = 0;
	PRR |= (1 << PRADC);
}

// Diagnostico de despertares

void indicar_despertar(void)
{
	#if DIAGNOSTICO

	PORTB |= (1 << PB1);
	_delay_ms(100);
	PORTB &= ~(1 << PB1);

	#endif
}

// Programa principal

int main(void)
{
	cli();

	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	// Pines D8, D9 y pulsador D2
	DDRB |= (1 << PB0) | (1 << PB1);
	PORTB &= ~((1 << PB0) | (1 << PB1));

	DDRD &= ~(1 << PD2);
	PORTD |= (1 << PD2);

	PCMSK2 |= (1 << PCINT18);
	PCIFR = (1 << PCIF2);
	PCICR |= (1 << PCIE2);

	// Desactivacion de temporizadores
	TCCR0B = 0;
	TCCR1B = 0;
	TCCR2B = 0;

	TIMSK0 = 0;
	TIMSK1 = 0;
	TIMSK2 = 0;

	// Desactivacion de ADC y USART
	ADCSRA = 0;
	UCSR0B = 0;

	DIDR0 |= (1 << ADC0D);
	ACSR |= (1 << ACD);

	// Reduccion de consumo mediante PRR
	PRR = (1 << PRADC) |
	(1 << PRUSART0) |
	(1 << PRSPI) |
	(1 << PRTIM0) |
	(1 << PRTIM1) |
	(1 << PRTIM2) |
	(1 << PRTWI);

	#if USAR_WDT
	iniciar_wdt();
	#endif

	sei();

	set_sleep_mode(SLEEP_MODE_PWR_DOWN);

	// Gestion de eventos y reposo

	while (1)
	{
		if (evento_wdt)
		{
			evento_wdt = 0;

			leer_adc();
			indicar_despertar();
		}

		if (evento_boton)
		{
			evento_boton = 0;
			indicar_despertar();
		}

		cli();

		if (!evento_wdt && !evento_boton)
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
	}
}
