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

//Funcion Puntero A funcion Estado
typedef void(*EstadoFunc)(void);

//Declaración de función de cada estado
void st_activo(void);
void st_standby(void);
void st_apagado(void);

//Variable global que guarda la dirección del estado actual
//'volatile' evita que el compilador optimice o descarte la variable si cambia en una ISR
volatile EstadoFunc estado_actual = st_activo;

// Variables de conteo de inactividad
volatile uint16_t contador_inactividad_s = 0;

void st_activo(void) {
	// Reset de hardware del Watchdog Timer (evita que el perro muerda y reinicie el chip)
	wdt_reset();
	
	// Aca van tareas como Mediciones, actualizacion del LCD, etc
	
	// Transición por inactividad: se reasigna la dirección del puntero
	if (contador_inactividad_s >= 10) {
		estado_actual = st_standby; // Apunta a la función st_standby
	}
}

void st_standby(void) {
	// Configurar SMCR para IDLE: Limpiar SM2, SM1, SM0 y activar SE (bit 0)
	SMCR = (1 << SE); // SM2:SM0 = 000 (Idle) y SE = 1

	// Ejecutar instrucción sleep 
	asm volatile("sleep");

	// Inmediatamente al despertar de una interrupción, deshabilitar SE por seguridad
	SMCR &= ~(1 << SE);

	// Evaluamos el paso al siguiente estado si la inactividad sigue aumentando
	if (contador_inactividad_s >= 30) {
		estado_actual = st_apagado;
	}
}

void st_apagado(void) {
	//Corte de energía por hardware en el registro PRR
	// Desactiva ADC, Timer1, Timer2 y SPI
	PRR |= (1 << PRADC) | (1 << PRTIM1) | (1 << PRTIM2) | (1 << PRSPI);

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

// ISR del WDT: Se activa cada 8 segundos en Power-Down
ISR(WDT_vect) {
	// Aquí el chip acaba de despertar.
	// Se puede levantar una bandera para hacer una medición rápida del sensor
}

// ISR de Recepción Serie (UART RX)
ISR(USART_RX_vect) {
	// Leer el byte del registro de datos de la USART (limpia la interrupción automáticamente)
	uint8_t rx_byte = UDR0;

	// Al recibir cualquier comando por Bluetooth, se reinicia la inactividad y volvemos a ACTIVO
	contador_inactividad_s = 0;
	estado_actual = st_activo;
}

// ISR del Botón Físico de Emergencia (INT0 en el pin PD2)
ISR(INT0_vect) {
	contador_inactividad_s = 0;
	estado_actual = st_activo;
}
int main(void)
{
	// Configuración inicial del WDT de seguridad a 2 segundos
	wdt_enable(WDTO_2S);
	// Habilitar interrupciones globales (Bit I del registro SREG)
	sei();

    while (1) 
    {
		// Ejecución directa de la dirección de memoria de la función actual
		estado_actual();
	}

	return 0;
}
   


