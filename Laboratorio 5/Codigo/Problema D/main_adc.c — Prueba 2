
#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <stdint.h>

// Nuestras librerias
#include "interrupciones.h"
#include "mediciones.h"

// ====================================
// PRUEBA 2: ADC NOISE REDUCTION
// ====================================

// Variables globales del ADC
volatile uint16_t valor_adc = 0;
volatile uint8_t adc_terminado = 0;

// Interrupcion cuando termina la conversion ADC
ISR(ADC_vect)
{
    valor_adc = ADC;
    adc_terminado = 1;
}

int main(void)
{
    // Configurar LED PB0 (D8) como salida
    DDRB |= (1 << PB0);
    PORTB &= ~(1 << PB0);

    // Inicializar modulos
    interrupciones_init();
    mediciones_init();

    // ====================================
    // CONFIGURACION DEL ADC
    // ====================================

    // Referencia AVcc (5 V), canal ADC0 (A0)
    ADMUX = (1 << REFS0);

    // Habilitar ADC e interrupcion
    // Prescaler 128
    ADCSRA = (1 << ADEN)  |
             (1 << ADIE)  |
             (1 << ADPS2) |
             (1 << ADPS1) |
             (1 << ADPS0);

    // Deshabilitar INT0 para que no despierte al Arduino
    EIMSK &= ~(1 << INT0);
    EIFR = (1 << INTF0);

    // Seleccionar modo ADC Noise Reduction
    set_sleep_mode(SLEEP_MODE_ADC);

    // Habilitar interrupciones globales
    sei();

    while (1)
    {
        // Apagar LED
        PORTB &= ~(1 << PB0);

        // Esperar pulsacion en D2
        while (PIND & (1 << PD2))
        {
        }

        // Esperar a soltar el boton
        while (!(PIND & (1 << PD2)))
        {
        }

        // ====================================
        // PREPARAR CONVERSION ADC
        // ====================================

        // Limpiar bandera de conversion
        adc_terminado = 0;

        // Desactivar disparo automatico
        ADCSRA &= ~(1 << ADATE);

        // Limpiar interrupcion ADC pendiente
        ADCSRA |= (1 << ADIF);

        // Desactivar interrupciones temporalmente
        cli();

        // Preparar entrada a Sleep
        sleep_enable();

        // Iniciar conversion ADC
        ADCSRA |= (1 << ADSC);

        // Habilitar interrupciones
        sei();

        // Entrar en ADC Noise Reduction
        sleep_cpu();

        // Desactivar Sleep al despertar
        sleep_disable();

        // Esperar a que termine la conversion
        while (!adc_terminado)
        {
        }

        // ====================================
        // MOSTRAR RESULTADO
        // ====================================

        // Iniciar señal de medicion
        mediciones_inicio();

        // Comprobar valor del potenciometro
        if (valor_adc >= 512)
        {
            // Potenciometro por encima de la mitad
            PORTB |= (1 << PB0);
        }
        else
        {
            // Potenciometro por debajo de la mitad
            PORTB &= ~(1 << PB0);
        }

        // Mostrar resultado durante 1 segundo
        _delay_ms(1000);

        // Apagar LED
        PORTB &= ~(1 << PB0);

        // Finalizar señal de medicion
        mediciones_fin();
    }
}
