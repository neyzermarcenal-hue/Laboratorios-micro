#include <avr/io.h>
#include <stdint.h>
#include "sensores.h"

// Configuracion de Medicion 
// Referencia ADC: 5 V nominales
#define VREF_MV 5000UL

// Divisor resistivo: 10k + 10k
// El voltaje de A0 es la mitad del voltaje de la bateria.
#define FACTOR_DIVISOR 2UL

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

	uint16_t lectura_adc = ADC;

	// Convertir ADC a milivoltios
	uint32_t voltaje_mv = ((uint32_t)lectura_adc * VREF_MV *FACTOR_DIVISOR) / 1023UL;
	
	bateria_dv = (decivoltios > 255U) ?
	255U : (uint8_t)decivoltios;

		bateria_dv = (uint8_t)voltaje_dv;


	if (bateria_baja == 0)
	{
		if (bateria_dv <= BATERIA_BAJA_DV)
		{
			bateria_baja = 1;
		}
	}
	else
	{
		if (bateria_dv >= BATERIA_RECUPERADA_DV)
		{
			bateria_baja = 0;
		}
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
	// Todavia no implementado
	return 0;
}

