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

//Definicion de funciones 

void timer1_init(void);
void reproducir_nota(uint16_t ocr_valor);
void apagar(void);
void botones_init(void);
int8_t obtener_nota_pulsada(void);

//Le asignamos al arreglo NOTA, el valor del registro 0CRnx

const uint16_t NOTAS[8]={476, 424, 378, 357, 318, 283, 252, 238};
	
//Cancion 1 - "El viejo" - La Vela Puerca

const uint16_t CANCION_1[] = {
	NOTAS[4], NOTAS[4], NOTAS[4], NOTAS[4], NOTAS[3], NOTAS[2], NOTAS[3], NOTAS[4]
};

const uint16_t DURACIONES_1[] = {
	200, 200, 200, 200, 200, 200, 200, 600
};

//Cancion 2 - "The Avengers Theme" - Alan Silvestri

const uint16_t CANCION_2[] = {
	NOTAS[4], NOTAS[4], NOTAS[4], NOTAS[7],
	NOTAS[6], NOTAS[5], NOTAS[4],
	NOTAS[2], NOTAS[4], NOTAS[7]
};

const uint16_t DURACIONES_2[] = {
	150, 150, 150, 600,
	300, 300, 600,
	300, 300, 800
};

// Cancion 3 - "Mamma Mia" - ABBA

const uint16_t CANCION_3[] = {
	NOTAS[4], NOTAS[4], NOTAS[3], NOTAS[2], NOTAS[0],
	NOTAS[1], NOTAS[2], NOTAS[3], NOTAS[2], NOTAS[1],
	NOTAS[0], NOTAS[0],
	NOTAS[1], NOTAS[2], NOTAS[1], NOTAS[0]
};

const uint16_t DURACIONES_3[] = {
	250, 250, 250, 250, 400,
	250, 250, 250, 250, 400,
	300, 300,
	250, 250, 250, 500
};
	
void timer1_init(void){
	//PB1 y PB2 como salida 
	DDRB |= (1 << PB1) | (1 << PB2); 
	//Configurar modo CTC
	TCCR1A = 0;
	TCCR1B = (1<<WGM12);
}

void reproducir_nota_piano(uint16_t ocr_valor) {
	OCR1A = ocr_valor;
	TCCR1A &= ~(1 << COM1B0);              // Desconecta el Buzzer 2
	TCCR1A |= (1 << COM1A0);               // Activa el Buzzer 1 
	TCCR1B |= (1 << CS11) | (1 << CS10);
}

void reproducir_nota_cancion(uint16_t ocr_valor) {
	OCR1A = ocr_valor;
	OCR1B = ocr_valor;
	TCCR1A &= ~(1 << COM1A0);              // Desconecta el Buzzer 1
	TCCR1A |= (1 << COM1B0);               // Activa el Buzzer 2 
	TCCR1B |= (1 << CS11) | (1 << CS10);
}

void silenciar(void) {
	TCCR1A &= ~((1 << COM1A0) | (1 << COM1B0));            // Desconecta ambos buzzers
	TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10));  // Detiene el Timer
}

//Lectura de botones (Piano Manual)

void botones_init(void){
	//PD2...PD7 Y PC0, PC1 entradas	
	DDRD &= ~((1 << PD2) | (1 << PD3) | (1 << PD4) | (1 << PD5) | (1 << PD6) | (1 << PD7));
	DDRC &= ~(( 1<< PC0) | (1 << PC1)); 
	
	PORTD |= (1 << PD2) | (1 << PD3) | (1 << PD4) | (1 << PD5) | (1 << PD6) | (1 << PD7);
	PORTC |= ( 1<< PC0) | (1 << PC1); 
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
void usart_init(void) {
	uint16_t ubrr = 103; // 9600 baudios para F_CPU = 16 MHz
	UBRR0H = (uint8_t)(ubrr >> 8);
	UBRR0L = (uint8_t)ubrr;
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);   
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); 
}

uint8_t usart_disponible(void) {
	return (UCSR0A & (1 << RXC0)); // Retorna 1 si hay un carácter esperando en el buffer
}

char usart_recibir(void) {
	while (!(UCSR0A & (1 << RXC0)));
	return UDR0;
}

//Funcion "Reproduccion de Canciones"

uint8_t esperar_cancelacion(uint16_t milisegundos){
	for (uint16_t =0;t<milisegundos; t+=10){
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

void reproducir_cancion(const uint16_t* melodia, const uint16_t* duraciones, uint8_t cantidad_notas){
	for (uint8_t i = 0; i< cantidad_notas; i++){
		uint16_t nota = melodia[i];
		uint16_t duracion = duraciones[i];
		
		reproducir_nota_cancion(nota);
		
		//Toca la nota durante el 85% del tiempo
		if(esperar_cancelacion(duracion * 85 / 100)){
			apagar();
			return; //Cancela e interrumpe inmediatamente
		}
			apagar();
			
			//Pausa del 15% entre notas para separarlas claramente
			if(esperar_cancelacion(duracion * 15 /100)){
				return;
			}
		}
	}

int main(void)
{
	timer1_init();
	botones_init();
	usart_init();
		
	int8_t nota_actual= -1;
 
    while (1) 
    {
		//Opcion recibida por UART
		if(usart_disponible()){
			char comando = usart_recibir();
			
			if(comando == 'C' || comando == 'c'){
				char seleccion = usart_recibir(); // Lee el número siguiente ('1', '2' o '3')
				
				if (sleccion == '1'){
					reproducir_cancion(CANCION_1, DURACIONES_1, sizeof(CANCION_1) / sizeof(CANCION_1[0]));
				}
				else if (seleccion == '2'){
					reproducir_cancion(CANCION_2, DURACIONES_2, sizeof(CANCION_2) / sizeof(CANCION_2[0]));
				}
				else if(seleccion == '3'){
					reproducir_cancion(CANCION_3, DURACIONES_3, sizeof(CANCION_3)/ sizeof(CANCION_3)[0]));
				}
			}
		}
		nota_actual = obtener_nota_pulsada();
		
		if(nota_actual !=-1){
			reproducir_nota_piano(NOTAS[nota_actual]);
    }else{
		apagar();
	}
	_delay_ms(10); //Antirrebote
	
	}
	return 0;
}

