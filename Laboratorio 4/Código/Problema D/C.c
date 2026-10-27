#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>


#define BAUD          9600
#define MY_UBRR       (F_CPU/16/BAUD-1)

#define WS2812_PIN    PB1    
#define NUM_LEDS      64     /* Matriz 8x8 */

#define JOY_SW        PD2    /* Pulsador del Joystick */
#define BUZZER_PIN    PB0    /* Buzzer activo/pasivo */

#define MAX_NIVEL     32     /* Longitud máxima de secuencia */
#define DEADZONE_LOW  350    /* Umbral inferior de zona muerta */
#define DEADZONE_HIGH 670    /* Umbral superior de zona muerta */

#define EE_MARCA_DIR  0x00
#define EE_MARCA_VAL  0xD4   /* Firma de validación EEPROM */
#define EE_HS_DIR     0x02   /* Dirección del High Score */

typedef enum {
	ST_INICIO,
	ST_GENERAR_SECUENCIA,
	ST_REPRODUCIR_SECUENCIA,
	ST_TURNO_JUGADOR,
	ST_EXITO_NIVEL,
	ST_GAME_OVER
} EstadoJuego;

typedef struct {
	uint8_t fila;
	uint8_t col;
} Posicion;

typedef struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} ColorRGB;

uint8_t leds_grb[NUM_LEDS * 3];

/* Paleta de colores para los elementos del juego */
const ColorRGB COLOR_CURSOR   = {255, 255, 255}; /* Blanco brillante */
const ColorRGB COLOR_OBJETIVO = {0, 255, 150};   /* Verde azulado */
const ColorRGB COLOR_CORRECTO = {0, 255, 0};     /* Verde */
const ColorRGB COLOR_ERROR    = {255, 0, 0};     /* Rojo */
const ColorRGB COLOR_APAGADO  = {0, 0, 0};
	
/*UART*/

void uart_init(uint16_t ubrr)
{
	UBRR0H = (uint8_t)(ubrr >> 8);
	UBRR0L = (uint8_t)ubrr;
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_print(const char *s)
{
	while (*s) {
		while (!(UCSR0A & (1 << UDRE0)));
		UDR0 = *s++;
	}
}

void uart_print_num(uint16_t num)
{
	char buf[6];
	int8_t i = 0;
	if (num == 0) {
		uart_print("0");
		return;
	}
	while (num > 0) {
		buf[i++] = '0' + (num % 10);
		num /= 10;
	}
	while (--i >= 0) {
		while (!(UCSR0A & (1 << UDRE0)));
		UDR0 = buf[i];
	}
}

/* EEPROM*/

void eeprom_write_byte(uint16_t addr, uint8_t data)
{
	while (EECR & (1 << EEPE));
	EEAR = addr;
	EEDR = data;
	cli();
	EECR = (1 << EEMPE);
	EECR = (1 << EEMPE) | (1 << EEPE);
	sei();
}

uint8_t eeprom_read_byte(uint16_t addr)
{
	while (EECR & (1 << EEPE));
	EEAR = addr;
	EECR |= (1 << EERE);
	return EEDR;
}

void eeprom_write_word(uint16_t addr, uint16_t val)
{
	eeprom_write_byte(addr, (uint8_t)(val & 0xFF));
	eeprom_write_byte(addr + 1, (uint8_t)(val >> 8));
}

uint16_t eeprom_read_word(uint16_t addr)
{
	uint8_t low = eeprom_read_byte(addr);
	uint8_t high = eeprom_read_byte(addr + 1);
	return (uint16_t)((high << 8) | low);
}

/* ============================ DRIVER WS2812B ============================== */

void ws2812_init(void)
{
	DDRB |= (1 << WS2812_PIN);
	PORTB &= ~(1 << WS2812_PIN);
}

void ws2812_render(void)
{
	uint8_t sreg = SREG;
	cli();

	uint16_t total_bytes = NUM_LEDS * 3;
	uint8_t *ptr = leds_grb;

	while (total_bytes--) {
		uint8_t b = *ptr++;
		for (uint8_t i = 0; i < 8; i++) {
			if (b & 0x80) {
				PORTB |= (1 << WS2812_PIN);
				asm volatile("rjmp .+0\n\trjmp .+0\n\trjmp .+0\n\trjmp .+0\n\tnop\n\t");
				PORTB &= ~(1 << WS2812_PIN);
				asm volatile("rjmp .+0\n\tnop\n\t");
				} else {
				PORTB |= (1 << WS2812_PIN);
				asm volatile("rjmp .+0\n\tnop\n\t");
				PORTB &= ~(1 << WS2812_PIN);
				asm volatile("rjmp .+0\n\trjmp .+0\n\trjmp .+0\n\trjmp .+0\n\tnop\n\t");
			}
			b <<= 1;
		}
	}

	SREG = sreg;
	_delay_us(60);
}

void ws2812_clear(void)
{
	for (uint16_t i = 0; i < NUM_LEDS * 3; i++) {
		leds_grb[i] = 0;
	}
	ws2812_render();
}

void ws2812_set_pixel(uint8_t fila, uint8_t col, ColorRGB color)
{
	if (fila >= 8 || col >= 8) return;
	
	uint16_t idx = ((uint16_t)fila * 8 + col) * 3;
	
	leds_grb[idx]     = color.g;
	leds_grb[idx + 1] = color.r;
	leds_grb[idx + 2] = color.b;
}

/*  ADC (JOYSTICK)  */

void adc_init(void)
{
	ADMUX = (1 << REFS0);
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t adc_read(uint8_t canal)
{
	ADMUX = (ADMUX & 0xF0) | (canal & 0x0F);
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
	return ADC;
}

/*  BUZZER */

void buzzer_init(void)
{
	DDRB |= (1 << BUZZER_PIN);
	PORTB &= ~(1 << BUZZER_PIN);
}

void tono(uint16_t freq_hz, uint16_t duracion_ms)
{
	uint32_t period_us = 1000000UL / freq_hz;
	uint32_t half_period = period_us / 2;
	uint32_t ciclos = ((uint32_t)duracion_ms * 1000UL) / period_us;

	for (uint32_t i = 0; i < ciclos; i++) {
		PORTB |= (1 << BUZZER_PIN);
		for (uint32_t d = 0; d < half_period / 10; d++) _delay_us(10);
		PORTB &= ~(1 << BUZZER_PIN);
		for (uint32_t d = 0; d < half_period / 10; d++) _delay_us(10);
	}
}

void sonido_exito(void)
{
	tono(523, 100); _delay_ms(50);
	tono(659, 100); _delay_ms(50);
	tono(784, 150);
}

void sonido_error(void)
{
	tono(200, 200); _delay_ms(50);
	tono(150, 400);
}

/* BOTÓN JOYSTICK */

void sw_init(void)
{
	DDRD &= ~(1 << JOY_SW);
	PORTD |= (1 << JOY_SW);
}

uint8_t sw_presionado(void)
{
	if (!(PIND & (1 << JOY_SW))) {
		_delay_ms(20);
		if (!(PIND & (1 << JOY_SW))) {
			while (!(PIND & (1 << JOY_SW)));
			_delay_ms(20);
			return 1;
		}
	}
	return 0;
}

/*LÓGICA DEL JUEGO*/

Posicion secuencia[MAX_NIVEL];
uint16_t nivel_actual = 1;
uint16_t record_historico = 0;
uint16_t semilla_random = 0;

uint16_t obtener_random(void)
{
	semilla_random = (semilla_random * 2053 + 13849) % 65535;
	return semilla_random;
}

void animacion_inicio(void)
{
	for (uint8_t r = 0; r < 8; r++) {
		for (uint8_t c = 0; c < 8; c++) {
			ColorRGB col = {r * 30, c * 30, (r + c) * 15};
			ws2812_set_pixel(r, c, col);
		}
	}
	ws2812_render();
	tono(440, 100); _delay_ms(100);
	tono(880, 200);
	_delay_ms(400);
	ws2812_clear();
}

void animacion_error(void)
{
	for (uint8_t i = 0; i < 3; i++) {
		for (uint8_t r = 0; r < 8; r++) {
			for (uint8_t c = 0; c < 8; c++) {
				if (r == c || r == (7 - c)) {
					ws2812_set_pixel(r, c, COLOR_ERROR);
					} else {
					ws2812_set_pixel(r, c, COLOR_APAGADO);
				}
			}
		}
		ws2812_render();
		tono(180, 100);
		_delay_ms(150);
		ws2812_clear();
		_delay_ms(150);
	}
}