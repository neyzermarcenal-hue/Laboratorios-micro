/*
 * Problema A.c
 *
 * Created: 12/10/2026 0:53:40
 * Author : cvarg
 */ 
#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define LCD_I2C_ADDR 0x27
#define LCD_BACKLIGHT 0x08
#define LCD_EN        0x04
#define LCD_RS        0x01

volatile uint8_t sample_flag = 1;
float punto_medio = 20.0f;
float temp_actual = 0.0f;

typedef enum {
	STATE_HEATER_ON,
	STATE_IDEAL,
	STATE_FAN_ON
} SystemState;

SystemState current_state = STATE_IDEAL;

// Driver TWI / I2C
void twi_init(void) {
	TWSR = 0x00;
	TWBR = 72;
}

void twi_start(void) {
	uint16_t timeout = 10000;
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)) && --timeout);
}

void twi_stop(void) {
	TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
	_delay_us(100);
}

void twi_write(uint8_t data) {
	uint16_t timeout = 10000;
	TWDR = data;
	TWCR = (1 << TWINT) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)) && --timeout);
}

// Driver LCD I2C
void lcd_i2c_send_nibble(uint8_t nibble) {
	twi_start();
	twi_write(LCD_I2C_ADDR << 1);
	twi_write(nibble | LCD_EN);
	_delay_us(2);
	twi_write(nibble & ~LCD_EN);
	_delay_us(50);
	twi_stop();
}

void lcd_i2c_send_byte(uint8_t val, uint8_t mode) {
	uint8_t high = val & 0xF0;
	uint8_t low = (val << 4) & 0xF0;
	lcd_i2c_send_nibble(high | mode | LCD_BACKLIGHT);
	lcd_i2c_send_nibble(low | mode | LCD_BACKLIGHT);
}

void lcd_command(uint8_t cmd) {
	lcd_i2c_send_byte(cmd, 0);
	if (cmd == 0x01 || cmd == 0x02) _delay_ms(2);
}

void lcd_char(char data) {
	lcd_i2c_send_byte(data, LCD_RS);
}

void lcd_string(const char* str) {
	while (*str) lcd_char(*str++);
}

void lcd_init(void) {
	twi_init();
	_delay_ms(50);
	lcd_i2c_send_nibble(0x30 | LCD_BACKLIGHT); _delay_ms(5);
	lcd_i2c_send_nibble(0x30 | LCD_BACKLIGHT); _delay_us(150);
	lcd_i2c_send_nibble(0x30 | LCD_BACKLIGHT); _delay_us(150);
	lcd_i2c_send_nibble(0x20 | LCD_BACKLIGHT); _delay_ms(2);
	lcd_command(0x28);
	lcd_command(0x0C);
	lcd_command(0x06);
	lcd_command(0x01);
	_delay_ms(2);
}

void lcd_set_cursor(uint8_t row, uint8_t col) {
	uint8_t addr = (row == 0) ? (0x80 + col) : (0xC0 + col);
	lcd_command(addr);
}

// Driver USART a bajo nivel
void uart_init(unsigned int baud) {
	unsigned int ubrr = F_CPU / 16 / baud - 1;
	UBRR0H = (unsigned char)(ubrr >> 8);
	UBRR0L = (unsigned char)ubrr;
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_transmit(unsigned char data) {
	while (!(UCSR0A & (1 << UDRE0))); // Espera buffer libre
	UDR0 = data;                       // Escritura directa al registro UDR0
}

void uart_send_string(const char* str) {
	while (*str) uart_transmit(*str++);
}

// Conversión float a ASCII sin bibliotecas estándar
void send_float_custom(float val, void (*output_func)(char)) {
	if (val < 0) { output_func('-'); val = -val; }
	uint16_t entero = (uint16_t)val;
	uint8_t decimal = (uint8_t)((val - (float)entero) * 10.0f + 0.5f);
	if (decimal >= 10) { decimal = 0; entero++; }
	
	if (entero >= 100) {
		output_func('0' + (entero / 100));
		output_func('0' + ((entero / 10) % 10));
		} else if (entero >= 10) {
		output_func('0' + (entero / 10));
	}
	output_func('0' + (entero % 10));
	output_func('.');
	output_func('0' + decimal);
}

void uart_send_float(float val) { send_float_custom(val, uart_transmit); }
void lcd_send_float(float val) { send_float_custom(val, lcd_char); }

// Driver ADC
void adc_init(void) {
	ADMUX = (1 << REFS1) | (1 << REFS0); // Vref 1.1V
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t adc_read(uint8_t channel) {
	ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
	return ADC;
}

// Lectura analógica simple
float read_temperature_lm35(void) {
	uint16_t samples[16];
	for (uint8_t i = 0; i < 16; i++) {
		samples[i] = adc_read(0);
		_delay_us(200);
	}
	
	// Ordenamiento por burbuja
	for (uint8_t i = 0; i < 15; i++) {
		for (uint8_t j = 0; j < 15 - i; j++) {
			if (samples[j] > samples[j + 1]) {
				uint16_t temp = samples[j];
				samples[j] = samples[j + 1];
				samples[j + 1] = temp;
			}
		}
	}
	
	// Promedio de las 10 muestras centrales
	uint32_t sum = 0;
	for (uint8_t i = 3; i < 13; i++) sum += samples[i];
	
	float adc_avg = (float)sum / 10.0f;
	float voltage = (adc_avg * 1.1f) / 1024.0f;
	return voltage * 100.0f;
}

void timer1_pwm_init(void) {
	DDRB |= (1 << PB1) | (1 << PB0);
	TCCR1A = (1 << WGM10);
	TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10);
	OCR1A = 0;
}

void timer0_init(void) {
	TCCR0A = (1 << WGM01);
	TCCR0B = (1 << CS01) | (1 << CS00);
	OCR0A = 249;
	TIMSK0 |= (1 << OCIE0A);
}

ISR(TIMER0_COMPA_vect) {
	static uint16_t count_5s = 0;
	if (++count_5s >= 5000) {
		sample_flag = 1;
		count_5s = 0;
	}
}

void update_system_control(float temp) {
	const char* action_str = "";
	uint8_t state_changed;
	
	do {
		state_changed = 0;
		switch (current_state) {
			case STATE_HEATER_ON:
			if (temp >= 18.0f) { current_state = STATE_IDEAL; state_changed = 1; }
			break;
			case STATE_IDEAL:
			if (temp < 18.0f) { current_state = STATE_HEATER_ON; state_changed = 1; }
			else if (temp >= 24.0f) { current_state = STATE_FAN_ON; state_changed = 1; }
			break;
			case STATE_FAN_ON:
			if (temp < 24.0f) { current_state = STATE_IDEAL; state_changed = 1; }
			break;
		}
	}

	switch (current_state) {
		case STATE_HEATER_ON:
		TCCR1A &= ~(1 << COM1A1);
		PORTB &= ~(1 << PB1);
		PORTB |= (1 << PB0);
		action_str = "Calefactor ON";
		break;
		case STATE_IDEAL:
		TCCR1A &= ~(1 << COM1A1);
		PORTB &= ~((1 << PB0) | (1 << PB1));
		action_str = "Ideal (Todo OFF)";
		break;
		case STATE_FAN_ON:
		TCCR1A |= (1 << COM1A1);
		PORTB &= ~(1 << PB0);
		OCR1A = 128;
		action_str = "Ventilador ON";
		break;
	}

	uart_send_string("TEMP:"); uart_send_float(temp);
	uart_send_string("|ACCION:"); uart_send_string(action_str);
	uart_send_string("|PM:"); uart_send_float(punto_medio);
	uart_send_string("\r\n");

	lcd_set_cursor(0, 0);
	lcd_string("Temp: ");
	lcd_send_float(temp);
	lcd_string(" C   ");
}

int main(void)
{
	uart_init(9600);
	lcd_init();
	adc_init();
	timer1_pwm_init();
	timer0_init();
	sei();
    /* Replace with your application code */
    while (1) 
    {
		if (sample_flag) {
			sample_flag = 0;
			temp_actual = read_temperature_lm35();
			update_system_control(temp_actual);
		}
    }
}

