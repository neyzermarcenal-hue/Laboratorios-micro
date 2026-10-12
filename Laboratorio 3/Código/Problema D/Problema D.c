/*
 * Problema D - Lab3.c
 *
 * Created: 4/10/2026 0:16:33
 * Author : cvarg
 */ 

/*
VALOR DE FRECUENCIAS:
DO = 262 Hz
RE = 294 Hz
MI = 330 Hz
FA = 349 Hz
SOL= 392 Hz
LA = 440 Hz
SI = 494 Hz
DO agudo = 523 Hz   
*/

#define F_CPU 16000000UL // Frecuencia de 16MHz
#include <avr/io.h>
#include <util/delay.h>

#define SILENCIO 255
#define PORCENTAJE_SONIDO 80 // 80% tono / 20% silencio inter-nota

// DIRECCIÓN I2C Y MÁSCARAS LCD 
#define LCD_I2C_ADDR   (0x27 << 1)
#define LCD_BACKLIGHT 0x08
#define LCD_ENABLE    0x04
#define LCD_RS        0x01

//Definicion de funciones 

void i2c_init(void);
uint8_t i2c_start(void);
void i2c_stop(void);
uint8_t i2c_write(uint8_t data);

void lcd_send_nibble(uint8_t nibble, uint8_t is_data);
void lcd_send_byte(uint8_t byte, uint8_t is_data);
void lcd_cmd(uint8_t cmd);
void lcd_data(uint8_t data);
void lcd_init(void);
void lcd_set_cursor(uint8_t fila, uint8_t columna);
void lcd_print(const char* str);
void preparar_modo_piano_lcd(void);
void actualizar_nota_lcd(int8_t nota);

void timer1_init(void);
void botones_init(void);
void usart_init(void);
uint8_t usart_disponible(void);
char usart_recibir(void);
void usart_enviar_texto(const char* str);
void mostrar_menu_serial(void);
void reproducir_nota_piano(uint16_t ocr_valor);
void reproducir_nota_cancion(uint16_t ocr_valor);
void silenciar(void);
int8_t obtener_nota_pulsada(void);

uint8_t esperar_cancelacion(uint16_t milisegundos);
uint8_t reproducir_cancion(const uint16_t* notas_base, const uint8_t* melodia, const uint16_t* duraciones, uint8_t total_notas);

int8_t obtener_nota_pulsada(void);

//Tabla de datos de notas

const uint16_t NOTAS[8]={476, 424, 378, 357, 318, 283, 252, 238};
	
//Cancion 1 - "Osito Gominola" (Prescaler 8)

const uint16_t NOTAS_GOMINOLA[5] = {2550, 2272, 1910, 1702, 1516};
const uint8_t MELODIA_GOMINOLA[32] = {
	1, 1, 1, 2,
	1, 1, 1, 1, 1, 1, 2,
	4, 4, 4, 4, 4, 4,
	3, 3, 3, 3, 3, 3, 3,
	4, 3, 2,
	1, 1, 0, 2, 1
};
const uint16_t DURACIONES_GOMINOLA[32] = {
	180, 180, 180, 180,
	180, 360, 180, 180, 180, 180, 180,
	180, 360, 180, 180, 180, 180,
	180, 180, 180, 180, 180, 180, 180,
	180, 180, 180,
	360, 180, 180, 180, 540
};

//Cancion 2 - "Mamma Mia" - Abba

const uint16_t NOTAS_MAMMA_MIA[4] = {3821, 3404, 3032, 2862};
const uint8_t MELODIA_MAMMA_MIA[36] = {
	0, 0, 1, 0,           // do do re do
	0, 0, 1, 2, 1, 0,     // do do re mi re do
	1, 0,                 // re do
	3, 3, 3, 3, 2, 0,     // fa fa fa fa mi do
	0, 0, 1, 0,           // do do re do
	0, 0, 1, 2, 1, 0,     // do do re mi re do
	1, 0,                 // re do
	3, 3, 3, 3, 2, 0      // fa fa fa fa mi do
};
const uint16_t DURACIONES_MAMMA_MIA[36] = {
	250, 250, 250, 500,           // do do re do
	200, 200, 200, 200, 250, 400, // do do re mi re do
	500, 800,                     // re do
	200, 200, 200, 200, 250, 800, // fa fa fa fa mi do
	250, 250, 250, 500,           // do do re do
	200, 200, 200, 200, 250, 400, // do do re mi re do
	500, 800,                     // re do
	200, 200, 200, 200, 250, 1000 // fa fa fa fa mi do
};

// Cancion 3 - "Gravity Falls" - ABBA

const uint16_t NOTAS_GRAVITY[6] = {2272, 1910, 1803, 1702, 1516, 1431};
const uint8_t MELODIA_GRAVITY[32] = {
	5, 3, 0, 3, 5, 3, 0, 3,
	5, 1, 0, 1, 5, 1, 0, 1,
	4, 2, 0, 2, 4, 2, 0, 2,
	4, 2, 0, 2, 4, 2, 0, 3
};
const uint16_t DURACIONES_GRAVITY[32] = {
	185, 185, 185, 185, 185, 185, 185, 185,
	185, 185, 185, 185, 185, 185, 185, 185,
	185, 185, 185, 185, 185, 185, 185, 185,
	185, 185, 185, 185, 185, 185, 185, 370
};
	
	//Driver I2C
	
void timer1_init(void){
	//PB1 y PB2 como salida 
	DDRB |= (1 << PB1) | (1 << PB2); 
	//Configurar modo CTC
	TCCR1A = 0;
	TCCR1B = (1<<WGM12);
}

void i2c_init(void) {
	TWSR = 0x00;
	TWBR = 72; // 100 kHz @ 16 MHz
}

uint8_t i2c_start(void) {
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
	uint16_t timeout = 2000;
	while (!(TWCR & (1 << TWINT))) {
		if (--timeout == 0) { TWCR = 0; return 0; }
	}
	uint8_t status = TWSR & 0xF8;
	if (status != 0x08 && status != 0x10) { TWCR = 0; return 0; }
	return 1;
}

void i2c_stop(void) {
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
	uint16_t timeout = 2000;
	while ((TWCR & (1 << TWSTO)) && --timeout);
	_delay_us(100);
}

uint8_t i2c_write(uint8_t data) {
	TWDR = data;
	TWCR = (1 << TWINT) | (1 << TWEN);
	uint16_t timeout = 2000;
	while (!(TWCR & (1 << TWINT))) {
		if (--timeout == 0) { TWCR = 0; return 0; }
	}
	uint8_t status = TWSR & 0xF8;
	if (status != 0x18 && status != 0x28) return 0;
	return 1;
}

// DRIVER LCD

void lcd_send_nibble(uint8_t nibble, uint8_t is_data) {
	if (!lcd_presente) return;
	
	uint8_t data = (nibble & 0xF0) | LCD_BACKLIGHT | (is_data ? LCD_RS : 0);
	
	if (!i2c_start()) return;
	if (!i2c_write(LCD_I2C_ADDR)) { i2c_stop(); return; }
	
	i2c_write(data);
	_delay_us(10);
	
	i2c_write(data | LCD_ENABLE);
	_delay_us(10);
	
	i2c_write(data & ~LCD_ENABLE);
	_delay_us(50);
	
	i2c_stop();
}

void lcd_send_byte(uint8_t byte, uint8_t is_data) {
	if (!lcd_presente) return;
	lcd_send_nibble(byte & 0xF0, is_data);
	lcd_send_nibble((byte << 4) & 0xF0, is_data);
}

void lcd_cmd(uint8_t cmd) {
	if (!lcd_presente) return;
	lcd_send_byte(cmd, 0);
	if (cmd == 0x01 || cmd == 0x02) _delay_ms(5);
}

void lcd_data(uint8_t data) {
	if (!lcd_presente) return;
	lcd_send_byte(data, 1);
}

void lcd_init(void) {
	i2c_init();
	_delay_ms(100);
	
	if (i2c_start()) {
		if (i2c_write(LCD_I2C_ADDR)) {
			lcd_presente = 1;
		}
		i2c_stop();
	}

	if (!lcd_presente) return;

	_delay_ms(100);
	lcd_send_nibble(0x30, 0); _delay_ms(10);
	lcd_send_nibble(0x30, 0); _delay_ms(5);
	lcd_send_nibble(0x30, 0); _delay_ms(5);
	lcd_send_nibble(0x20, 0); _delay_ms(5);

	lcd_cmd(0x28); _delay_ms(2);
	lcd_cmd(0x0C); _delay_ms(2);
	lcd_cmd(0x01); _delay_ms(10);
	lcd_cmd(0x06); _delay_ms(2);
}

void lcd_set_cursor(uint8_t fila, uint8_t columna) {
	if (!lcd_presente) return;
	uint8_t addr = (fila == 0) ? (0x80 + columna) : (0xC0 + columna);
	lcd_cmd(addr);
}

void lcd_print(const char* str) {
	if (!lcd_presente) return;
	while (*str) lcd_data(*str++);
}

void preparar_modo_piano_lcd(void) {
	if (!lcd_presente) return;
	lcd_set_cursor(0, 0);
	lcd_print("Modo: Piano     ");
	lcd_set_cursor(1, 0);
	lcd_print("Nota: ---       ");
}

void actualizar_nota_lcd(int8_t nota) {
	if (!lcd_presente) return;
	lcd_set_cursor(1, 0);
	if (nota == 0)      lcd_print("Nota: DO        ");
	else if (nota == 1) lcd_print("Nota: RE        ");
	else if (nota == 2) lcd_print("Nota: MI        ");
	else if (nota == 3) lcd_print("Nota: FA        ");
	else if (nota == 4) lcd_print("Nota: SOL       ");
	else if (nota == 5) lcd_print("Nota: LA        ");
	else if (nota == 6) lcd_print("Nota: SI        ");
	else if (nota == 7) lcd_print("Nota: DO AGUDO  ");
	else                lcd_print("Nota: ---       ");
}

//Generacion de audio

void timer1_init(void) {
	DDRB |= (1 << PB1) | (1 << PB2); // Salidas habilitadas para Pin 9 y Pin 10
	TCCR1A = 0;
	TCCR1B = (1 << WGM12);          // Modo CTC
}

void reproducir_nota_piano(uint16_t ocr_valor) {
    TCNT1 = 0;
    OCR1A = ocr_valor;
    OCR1B = ocr_valor;
    TCCR1A = (1 << COM1A0) | (1 << COM1B0); // Activa ambos canales PB1 y PB2
    TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10); // Prescaler 64
}

void reproducir_nota_cancion(uint16_t ocr_valor) {
    TCNT1 = 0;
    OCR1A = ocr_valor;
    OCR1B = ocr_valor;
    TCCR1A = (1 << COM1A0) | (1 << COM1B0); // Activa ambos canales PB1 y PB2
    TCCR1B = (1 << WGM12) | (1 << CS11);    // Prescaler 8
}
}

void silenciar(void) {
	TCCR1A &= ~((1 << COM1A0) | (1 << COM1B0)); // Desactiva la salida de ambos canales
	TCCR1B = (1 << WGM12);
	PORTB &= ~((1 << PB1) | (1 << PB2));
	TCNT1 = 0;
}



//Lectura de botones (Piano Manual)

void botones_init(void){
	//PD2...PD7 Y PC0, PC1 entradas	
	DDRD &= ~((1 << PD2) | (1 << PD3) | (1 << PD4) | (1 << PD5) | (1 << PD6) | (1 << PD7));
	DDRC &= ~(( 1<< PC0) | (1 << PC1)); 
	
	PORTD |= (1 << PD2) | (1 << PD3) | (1 << PD4) | (1 << PD5) | (1 << PD6) | (1 << PD7);
	PORTC |= ( 1<< PC0) | (1 << PC1); 
}

void usart_init(void) {
	uint16_t ubrr = 103; // 9600 baudios @ F_CPU = 16 MHz
	UBRR0H = (uint8_t)(ubrr >> 8);
	UBRR0L = (uint8_t)ubrr;
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

int8_t obtener_nota_pulsada(void){
	if (!(PIND & (1 << PD2))) return 0; // Do
	if (!(PIND & (1 << PD3))) return 1; // Re
	if (!(PIND & (1 << PD4))) return 2; // Mi
	if (!(PIND & (1 << PD5))) return 3; // Fa
	if (!(PIND & (1 << PD6))) return 4; // Sol
	if (!(PIND & (1 << PD7))) return 5; // La
	if (!(PINC & (1 << PC0))) return 6; // Si
	if (!(PINC & (1 << PC1))) return 7; // Do agudo
	
	return -1; //NIngun boton presionado 
}

//Modulo USART

uint8_t usart_disponible(void) {
	return (UCSR0A & (1 << RXC0));
}

char usart_recibir(void) {
	while (!(UCSR0A & (1 << RXC0)));
	return UDR0;
}

void usart_enviar_texto(const char* str) {
	while (*str) {
		while (!(UCSR0A & (1 << UDRE0)));
		UDR0 = *str++;
	}
}

void mostrar_menu_serial(void) {
	usart_enviar_texto("\r\n=================================\r\n");
	usart_enviar_texto(" MENU DE CANCIONES (Escribe 1-3):\r\n");
	usart_enviar_texto("  1 - Osito Gominola\r\n");
	usart_enviar_texto("  2 - Mamma Mia (ABBA)\r\n");
	usart_enviar_texto("  3 - Gravity Falls\r\n");
	usart_enviar_texto("  S - Detener reproduccion\r\n");
	usart_enviar_texto("=================================\r\n> ");
}


//Funcion "Reproduccion de Canciones"

uint8_t esperar_cancelacion(uint16_t milisegundos){
	for (uint16_t = 0 ;t < milisegundos; t += 10){
		_delay_ms(10);	
		if (usart_disponible()){
			char c=usart_recibir();
			if (c=='S' || c=='s' || c=='0'){
				return 1; //Se solicito detener la cancion
			}
		}
	}
	return 0; //Finalizo la espera normalmente
}

uint8_t reproducir_cancion(const uint16_t* notas_base, const uint8_t* duraciones,const uint8_t total_notas){
	for (uint8_t i = 0; i< total_notas; i++){
		uint8_t nota_idx = melodia[i];
		uint16_t duracion = duraciones[i];
		
   if (nota_idx == SILENCIO) {
	   silenciar();
	   if (esperar_cancelacion(duracion)) {
		   silenciar();
		   return 1;
	   }
	   } else {
	   uint16_t t_sonido = (duracion * PORCENTAJE_SONIDO) / 100;
	   uint16_t t_silencio = duracion - t_sonido;

	   reproducir_nota_cancion(notas_base[nota_idx]);
	   if (esperar_cancelacion(t_sonido)) {
		   silenciar();
		   return 1;
	   }
	   
	   silenciar();
	   if (esperar_cancelacion(t_silencio)) {
		   return 1;
	   }
   }
   }
   silenciar();
   return 0;

int main(void)
{
	timer1_init();
	botones_init();
	usart_init();
	lcd_init();
		
	mostrar_menu_serial();
	preparar_modo_piano_lcd();

	int8_t nota_actual= -1;
 
    while (1) 
    {
		//Opcion recibida por UART
		if(usart_disponible()){
			char c = usart_recibir();
			
			if(c != '\r' && c != '\n' && c != ' '){
				uint8_t cancelada = 0;	
				
	             if (c == '1') {
		             usart_enviar_texto("\r\n[Sonando] 1- Osito Gominola...\r\n");
		             lcd_set_cursor(0, 0); lcd_print("C1: Osito Gom.  ");
		             lcd_set_cursor(1, 0); lcd_print("Sonando...      ");

		             cancelada = reproducir_cancion(NOTAS_GOMINOLA, MELODIA_GOMINOLA, DURACIONES_GOMINOLA, sizeof(MELODIA_GOMINOLA));
		             
		             lcd_set_cursor(1, 0);
		             if (cancelada) lcd_print("Detenida!       ");
		             else           lcd_print("Finalizada!     ");
		             _delay_ms(1000);
		             
		             preparar_modo_piano_lcd();
		             mostrar_menu_serial();
		             nota_anterior = -1;
	             }
	             else if (c == '2' || c == 'c' || c == 'C') {
		             usart_enviar_texto("\r\n[Sonando] 2- Mamma Mia (ABBA)...\r\n");
		             lcd_set_cursor(0, 0); lcd_print("C2: Mamma Mia   ");
		             lcd_set_cursor(1, 0); lcd_print("Sonando...      ");

		             cancelada = reproducir_cancion(NOTAS_MAMMA_MIA, MELODIA_MAMMA_MIA, DURACIONES_MAMMA_MIA, sizeof(MELODIA_MAMMA_MIA));
		             
		             lcd_set_cursor(1, 0);
		             if (cancelada) lcd_print("Detenida!       ");
		             else           lcd_print("Finalizada!     ");
		             _delay_ms(1000);
		             
		             preparar_modo_piano_lcd();
		             mostrar_menu_serial();
		             nota_anterior = -1;
	             }
	             else if (c == '3') {
		             usart_enviar_texto("\r\n[Sonando] 3- Gravity Falls...\r\n");
		             lcd_set_cursor(0, 0); lcd_print("C3:Gravity Falls");
		             lcd_set_cursor(1, 0); lcd_print("Sonando...      ");

		             cancelada = reproducir_cancion(NOTAS_GRAVITY, MELODIA_GRAVITY, DURACIONES_GRAVITY, sizeof(MELODIA_GRAVITY));
		             
		             lcd_set_cursor(1, 0);
		             if (cancelada) lcd_print("Detenida!       ");
		             else           lcd_print("Finalizada!     ");
		             _delay_ms(1000);
		             
		             preparar_modo_piano_lcd();
		             mostrar_menu_serial();
		             nota_anterior = -1;
	             }
             }
        }
            // 2. Modo Piano Manual
        int8_t lectura1 = obtener_nota_pulsada();
 
          if (lectura1 != nota_anterior) {
	          _delay_ms(20); // Antirrebote
	          int8_t lectura2 = obtener_nota_pulsada();
	 
	          if (lectura1 == lectura2) {
	             nota_anterior = lectura2;
		 
		         if (lectura2 != -1) {
			         reproducir_nota_piano(NOTAS_PIANO[lectura2]);
			    } else {
			        silenciar();
		        }
		 
		 actualizar_nota_lcd(lectura2);
	 }
 }

      _delay_ms(10);
 }

 return 0;
 }	


