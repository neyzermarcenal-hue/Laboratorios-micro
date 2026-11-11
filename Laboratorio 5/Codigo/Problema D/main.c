#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <util/delay.h>
#include <avr/sleep.h>

// Nuestras librerias
#include "sleep.h"
#include "interrupciones.h"
#include "perifericos.h"
#include "watchdog.h"
#include "reloj.h"
#include "mediciones.h"

// Seleccion de experimentos
#define PRUEBA_IDLE 1
#define PRUEBA_ADC_NOISE 2
#define PRUEBA_POWER_DOWN 3
#define PRUEBA_PERIFERICOS 4
#define PRUEBA_WDT 5
#define PRUEBA_RELOJ 6

// Elegimos la prueba
#define PRUEBA_ACTUAL PRUEBA_ADC_NOISE

volatile uint16_t valor_adc = 0;
volatile uint8_t adc_terminado = 0;

// Interrupción cuando termina la conversión ADC
ISR(ADC_vect)
{
    valor_adc = ADC;
    adc_terminado = 1;
}

int main(void)
{
    // Configurar LED PB0 como salida
    DDRB |= (1 << PB0);
    PORTB &= ~(1 << PB0);

    // Inicializar modulos
    interrupciones_init();
    mediciones_init();
    // Configuración del ADC
    ADMUX = (1 << REFS0);  // Referencia AVcc (5 V), canal ADC0 (A0)

    ADCSRA = (1 << ADEN)  |  // Habilita el ADC
         (1 << ADIE)  |  // Habilita interrupción del ADC
         (1 << ADPS2) |  // Prescaler 128
         (1 << ADPS1) |
         (1 << ADPS0);

    // Configurar modo Sleep Idle
    set_sleep_mode(SLEEP_MODE_IDLE);

    // Habilitar interrupciones globales
    sei();

   
    while (1)
    {
         // ====================================
        // PRUEBA 1: SLEEP IDLE
        // ====================================
        
        if (PRUEBA_ACTUAL == PRUEBA_IDLE)
        {
            // Apagar LED antes de dormir
            PORTB &= ~(1 << PB0);

            // Esperar a que el boton este suelto
            while (!(PIND & (1 << PD2)))
            {
            }

            // Limpiar evento anterior
            boton_presionado();

            // Desactivar interrupciones globales
            cli();

            // Deshabilitar INT0 temporalmente
            EIMSK &= ~(1 << INT0);

            // Limpiar interrupcion pendiente
            EIFR = (1 << INTF0);

            // Preparar entrada a Sleep
            sleep_enable();
    
            // Habilitar INT0
            EIMSK |= (1 << INT0);

            sei();
            sleep_cpu();
            sleep_disable();

            // El Arduino desperto
            mediciones_inicio();
            PORTB |= (1 << PB0);

            // Mantener LED encendido 500 ms
            _delay_ms(500);

            // Apagar LED y señal de medicion
            PORTB &= ~(1 << PB0);
            mediciones_fin();

            // Esperar que suelte el boton
            while (!(PIND & (1 << PD2)))
            {
            }

            // Limpiar evento anterior
            boton_presionado();
        }
        // ====================================
        // PRUEBA 2: ADC NOISE REDUCTION
        // ====================================
        else if (PRUEBA_ACTUAL == PRUEBA_ADC_NOISE)
        {
            PORTB &= ~(1 << PB0);

            // Evitar que INT0 despierte al Arduino
            EIMSK &= ~(1 << INT0);
            EIFR = (1 << INTF0);

            // Esperar pulsación en D2
            while (PIND & (1 << PD2)) {}

            // Esperar a soltar el botón
            while (!(PIND & (1 << PD2))) {}

            
            // Preparar nueva conversión
            adc_terminado = 0;

            // Desactivar disparo automático
            ADCSRA &= ~(1 << ADATE);

            // Limpiar interrupción ADC pendiente
            ADCSRA |= (1 << ADIF);

            // Seleccionar modo ADC Noise Reduction
            set_sleep_mode(SLEEP_MODE_ADC);

            // Desactivar interrupciones durante la preparación
            cli();

            // Preparar entrada a Sleep
            sleep_enable();

            // Iniciar conversión ADC
            ADCSRA |= (1 << ADSC);

            // Habilitar interrupciones
            sei();

            // Dormir mientras el ADC convierte
            sleep_cpu();

            // Desactivar Sleep al despertar
            sleep_disable();

            // Esperar a que termine la conversión
            while (!adc_terminado) {}

            // Indicar que terminó la conversión
            mediciones_inicio();

            // Comprobar el valor del potenciómetro
            if (valor_adc >= 512)
            {
                // Potenciómetro por encima de la mitad
                PORTB |= (1 << PB0);
            }
            else
            {
                // Potenciómetro por debajo de la mitad
                PORTB &= ~(1 << PB0);
            }
            
            // Mostrar resultado durante 1 segundo
            _delay_ms(1000);
            
            // Apagar LED
            PORTB &= ~(1 << PB0);
            mediciones_fin();
         }
    }  
}

