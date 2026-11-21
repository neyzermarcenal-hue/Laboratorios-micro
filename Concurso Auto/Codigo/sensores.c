#include <avr/io.h>
#include <stdint.h>
#include "sensores.h"

static uint8_t bateria_dv = 0;
static uint8_t bateria_baja = 0;

void sensores_init(void)
{
	#if MONITOREAR_BATERIA

	// Referencia AVcc, canal ADC0 (A0)
	ADMUX = (1 << REFS0);

	// Habilitar ADC, prescaler 128
	ADCSRA = (1 << ADEN) |
	(1 << ADPS2) |
	(1 << ADPS1) |
	(1 << ADPS0);

	#endif
}

void sensores_tarea(void)
{
	#if MONITOREAR_BATERIA

	// Iniciar conversion ADC
	ADCSRA |= (1 << ADSC);

	while (ADCSRA & (1 << ADSC))
	{
	}

	uint16_t adc = ADC;

	// Ejemplo: divisor 10k / 4.7k, Vref=5 V
	// Convertir a decivoltios
	uint16_t decivoltios =
	(uint16_t)(((uint32_t)adc * 1527UL)
	/ 10000UL);

	bateria_dv = (decivoltios > 255U) ?
	255U : (uint8_t)decivoltios;

	// Umbrales orientativos SOLO para bateria 2S.
	// Deben adaptarse a la bateria real.
	if (bateria_baja)
	{
		if (bateria_dv >= 68U)
		bateria_baja = 0;
	}
	else if (bateria_dv <= 66U)
	{
		bateria_baja = 1;
	}

	#endif
}

uint8_t sensores_bateria_dv(void)
{
	return bateria_dv;
}

uint8_t sensores_bateria_baja(void)
{
	return bateria_baja;
}

uint8_t sensores_objeto_agarrado(void)
{
	// Pendiente: final de carrera real.
	return 0;
}

