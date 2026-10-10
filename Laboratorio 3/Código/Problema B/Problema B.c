
#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdio.h>

// Configuracion general del sistema
#define MOTOR_HABILITADO 1
#define AUTO_HABILITADO 0
#define POT_MIN 200
#define POT_MAX 820
#define TOLERANCIA 25

// Relacion entre el sentido del motor y el mecanismo
#define IN1_ABRE 1
#define POT_SUBE_CON_IN1 1
#define IN1_ES_HORARIO 1

// Configuracion del PWM y tiempos de seguridad
#define PWM_TOP 1999
#define MAX_GIRO_MS 1500
#define SIN_MOVIMIENTO_MS 350
#define PRUEBA_MS 80
#define PWM_PRUEBA 85

volatile uint16_t tiempo_ms = 0;
volatile uint16_t setpoint_rx = 0;
volatile uint8_t nuevo_setpoint = 0;
volatile char comando_rx = 0;

// Comunicacion UART
// Basado en la clase USART-LUT y el ejemplo UART de Yisus
void UART_init(void)
{
	UCSR0A = 0;
	UBRR0H = 0;
	UBRR0L = 103; // 9600 baudios con 16 MHz

	// Habilita transmision, recepcion e interrupcion
	UCSR0B = (1 << TXEN0) |
	(1 << RXEN0) |
	(1 << RXCIE0);

	// 8 bits de datos, sin paridad y 1 bit de parada
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

// Envia un caracter por UART
void UART_sendChar(char dato)
{
	// Espera hasta que el registro este disponible
	while (!(UCSR0A & (1 << UDRE0)));

	UDR0 = dato;
}

// Envia un texto completo por UART.
void UART_sendString(const char *texto)
{
	while (*texto)
	{
		UART_sendChar(*texto);
		texto++;
	}
}

// Interrupcion para recibir el setpoint
// Permite recibir datos mientras el programa sigue funcionando
ISR(USART_RX_vect)
{
	static uint16_t numero = 0;
	static uint8_t hay_numero = 0;
	static uint8_t invalido = 0;

	char dato = UDR0;

	if (dato >= '0' && dato <= '9')
	{
		uint8_t digito = dato - '0';
		hay_numero = 1;

		// Comprueba que el numero no supere 1023
		if (!invalido)
		{
			if (numero > 102 ||
			(numero == 102 && digito > 3))
			{
				invalido = 1;
			}
			else
			{
				numero = numero * 10 + digito;
			}
		}
	}
	else if (dato == '\r' || dato == '\n')
	{
		// Al presionar Enter se guarda el setpoint
		if (hay_numero && !invalido)
		{
			setpoint_rx = numero;
			nuevo_setpoint = 1;
		}

		numero = 0;
		hay_numero = 0;
		invalido = 0;
	}
	else if (!hay_numero && (dato == 'D' || dato == 'I'))
	{
		// Comandos para comprobar los dos sentidos del motor.
		comando_rx = dato;
	}
	else
	{
		invalido = 1;
	}
}

// Conversion analogico-digital
// Basado en ADC.c de Yisus y la clase Motores-ADC
void ADC_init(void)
{
	// A1 y A2 se utilizan como entradas analogicas
	DDRC &= ~((1 << PC1) | (1 << PC2));
	PORTC &= ~((1 << PC1) | (1 << PC2));

	// Selecciona AVCC como referencia de voltaje.
	ADMUX = (1 << REFS0);

	// Habilita el ADC y configura prescaler 128.
	ADCSRA = (1 << ADEN) |
	(1 << ADPS2) |
	(1 << ADPS1) |
	(1 << ADPS0);
}

// Lee un canal ADC y devuelve un valor entre 0 y 1023.
uint16_t ADC_read(uint8_t canal)
{
	// Selecciona el canal sin cambiar la referencia.
	ADMUX = (ADMUX & 0xF0) | (canal & 0x07);

	// Inicia la conversion.
	ADCSRA |= (1 << ADSC);

	// Espera hasta que termine.
	while (ADCSRA & (1 << ADSC));

	return ADC;
}

// Generacion de PWM mediante Timer1.
// Basado en pwm.c de Yisus y la clase Introduccion a C-PWM.
void PWM_init(void)
{
	// PB1 (D9) genera PWM para el L298N.
	// PB0 (D8) y PD7 (D7) controlan el sentido.
	DDRB |= (1 << PB1) | (1 << PB0);
	DDRD |= (1 << PD7);

	PORTB &= ~(1 << PB0);
	PORTD &= ~(1 << PD7);

	// PWM rapido, modo 14, con TOP en ICR1.
	ICR1 = PWM_TOP;
	OCR1A = 0;

	TCCR1A = (1 << COM1A1) | (1 << WGM11);
	TCCR1B = (1 << WGM13) |
	(1 << WGM12) |
	(1 << CS11);

	// Habilita la interrupcion por desbordamiento.
	TIMSK1 = (1 << TOIE1);
}

// Timer1 genera PWM a 1 kHz.
// Cada ciclo dura 1 ms y sirve para llevar el tiempo.
ISR(TIMER1_OVF_vect)
{
	tiempo_ms++;
}

// Control del motor a traves del puente H.
// Sentido 1 usa IN1, -1 usa IN2 y 0 detiene el motor.
void MOTOR_control(int8_t sentido, uint8_t pwm)
{
	// Primero corta el PWM antes de cambiar las entradas.
	OCR1A = 0;

	if (sentido == 0 || pwm == 0)
	{
		PORTB &= ~(1 << PB0);
		PORTD &= ~(1 << PD7);
		return;
	}

	if (sentido == 1)
	{
		PORTB |= (1 << PB0);
		PORTD &= ~(1 << PD7);
	}
	else
	{
		PORTB &= ~(1 << PB0);
		PORTD |= (1 << PD7);
	}

	// Convierte el porcentaje al valor usado por Timer1.
	OCR1A = ((uint32_t)PWM_TOP * pwm) / 100;
}

// Comprueba los limites de posicion del potenciometro.
// Evita seguir girando hacia un extremo del mecanismo.
uint8_t MOTOR_limite(int8_t sentido, uint16_t pot)
{
	uint8_t aumenta;

	if (sentido == 1)
	aumenta = POT_SUBE_CON_IN1;
	else
	aumenta = !POT_SUBE_CON_IN1;

	if (aumenta && pot >= POT_MAX)
	return 1;

	if (!aumenta && pot <= POT_MIN)
	return 1;

	return 0;
}

int main(void)
{
	uint16_t ldr = 0;
	uint16_t pot = 0;
	uint16_t setpoint = 0;
	uint16_t ahora;

	uint16_t ultimo_control = 0;
	uint16_t ultimo_envio = 0;
	uint16_t inicio_giro = 0;
	uint16_t ultimo_movimiento = 0;
	uint16_t pot_anterior = 0;
	uint16_t inicio_prueba = 0;
	uint16_t pot_inicio_prueba = 0;

	uint8_t setpoint_valido = 0;
	uint8_t pwm = 0;
	uint8_t bloqueo = 0;
	uint8_t prueba_activa = 0;

	int8_t sentido = 0;
	int8_t sentido_anterior = 0;
	int8_t sentido_prueba = 0;
	int16_t error = 0;

	uint16_t diferencia;
	uint16_t cambio_pot;

	char mensaje[100];
	char comando = 0;
	const char *nombre_sentido;

	// Inicializa todos los perifericos.
	PWM_init();
	MOTOR_control(0, 0);
	ADC_init();
	UART_init();

	// Habilita las interrupciones.
	sei();

	UART_sendString("Control de iluminacion iniciado\r\n");
	UART_sendString("Ingresar setpoint entre 0 y 1023\r\n");

	while (1)
	{
		// Copia el tiempo y los datos recibidos por UART.
		// Las interrupciones se deshabilitan durante esta lectura.
		uint8_t estado = SREG;
		cli();

		ahora = tiempo_ms;

		if (nuevo_setpoint)
		{
			setpoint = setpoint_rx;
			setpoint_valido = 1;
			nuevo_setpoint = 0;
		}

		if (comando_rx != 0)
		{
			comando = comando_rx;
			comando_rx = 0;
		}

		SREG = estado;

		// Actualiza las lecturas y el control cada 20 ms.
		if ((uint16_t)(ahora - ultimo_control) >= 20)
		{
			ultimo_control = ahora;

			// ADC1 mide la iluminacion.
			// ADC2 mide la posicion del mecanismo.
			ldr = ADC_read(1);
			pot = ADC_read(2);

			// Calcula la diferencia entre la luz deseada y real.
			error = (int16_t)setpoint - (int16_t)ldr;

			sentido = 0;
			pwm = 0;

			// Activa un movimiento corto con D o I.
			if ((comando == 'D' || comando == 'I') && !prueba_activa)
			{
				inicio_prueba = ahora;
				pot_inicio_prueba = pot;
				sentido_prueba = (comando == 'D') ? 1 : -1;
				prueba_activa = 1;
			}
			comando = 0;

			if (prueba_activa)
			{
				if ((uint16_t)(ahora - inicio_prueba) >= PRUEBA_MS)
				{
					prueba_activa = 0;
					MOTOR_control(0, 0);

					snprintf(mensaje, sizeof(mensaje),
					"PRUEBA=%c;POT_INI=%u;POT_FIN=%u;CAMBIO=%d\r\n",
					(sentido_prueba == 1) ? 'D' : 'I',
					pot_inicio_prueba, pot, (int)pot - (int)pot_inicio_prueba);

					UART_sendString(mensaje);
				}
				else
				{
					sentido = sentido_prueba;
					pwm = PWM_PRUEBA;
				}
			}

			// Comprueba los limites durante la prueba.
			if (prueba_activa && sentido != 0 && MOTOR_limite(sentido, pot))
			{
				prueba_activa = 0;
				sentido = 0;
				pwm = 0;
				MOTOR_control(0, 0);

				UART_sendString("Movimiento detenido por limite\r\n");
			}

			// Control automatico cuando existe un setpoint valido.
			if (MOTOR_HABILITADO && AUTO_HABILITADO && setpoint_valido && !bloqueo && !prueba_activa)
			{
				// Si falta iluminacion, abre el mecanismo.
				if (error > TOLERANCIA)
				sentido = IN1_ABRE ? 1 : -1;

				// Si sobra iluminacion, cierra el mecanismo.
				else if (error < -TOLERANCIA)
				sentido = IN1_ABRE ? -1 : 1;

				if (sentido != 0)
				{
					diferencia = (error < 0) ? -error : error;

					// Ajusta el PWM segun la diferencia de luz.
					pwm = 70 + diferencia / 30;

					if (pwm > 95)
					pwm = 95;
				}

				// Comprueba los limites del potenciometro.
				if (sentido != 0 && MOTOR_limite(sentido, pot))
				{
					sentido = 0;
					pwm = 0;
				}

				// Introduce una parada antes de invertir el giro.
				if (sentido != 0 &&
				sentido_anterior != 0 &&
				sentido != sentido_anterior)
				{
					sentido = 0;
					pwm = 0;
				}

				// Comprueba que el motor no quede girando sin control.
				if (sentido != 0)
				{
					if (sentido != sentido_anterior)
					{
						inicio_giro = ahora;
						ultimo_movimiento = ahora;
						pot_anterior = pot;
					}

					cambio_pot = (pot > pot_anterior)
					? pot - pot_anterior
					: pot_anterior - pot;

					// Actualiza el tiempo si cambia la posicion.
					if (cambio_pot >= 3)
					{
						ultimo_movimiento = ahora;
						pot_anterior = pot;
					}

					// Detiene el movimiento si supera el tiempo
					// o si el potenciometro deja de variar.
					if ((uint16_t)(ahora - inicio_giro) >= MAX_GIRO_MS ||
					(uint16_t)(ahora - ultimo_movimiento) >= SIN_MOVIMIENTO_MS)
					{
						bloqueo = 1;
						sentido = 0;
						pwm = 0;
					}
				}
			}

			// Aplica el sentido y el PWM calculados.
			MOTOR_control(sentido, pwm);
			sentido_anterior = sentido;
		}

		// Envia las variables solicitadas cada 300 ms.
		if ((uint16_t)(ahora - ultimo_envio) >= 300)
		{
			ultimo_envio = ahora;

			nombre_sentido = "PARADO";

			if (sentido == 1)
			nombre_sentido = "IN1";

			else if (sentido == -1)
			nombre_sentido = "IN2";

			snprintf(mensaje, sizeof(mensaje),
			"SP=%u;LDR=%u;POT=%u;ERROR=%d;PWM=%u;DIR=%s\r\n",
			setpoint, ldr, pot, (int)error,
			pwm, nombre_sentido);

			UART_sendString(mensaje);
		}
	}
}
