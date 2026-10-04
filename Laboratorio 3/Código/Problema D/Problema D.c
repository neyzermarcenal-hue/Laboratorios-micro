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

#define F_CPU 160000000UL // Frecuencia de 16MHz
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
	
void timer1_init(void){
	//PB1 como salida 
	DDRB |= (1 << PB1);
	//Configurar modo CTC
	TCCR1A = 0;
	TCCR1B = (1<<WGM12);
}

void reproducir_nota(uint16_t ocr_valor){
	OCR1A = ocr_valor;
	
	TCCR1A |= (1<<COM1A0);
	TCCR1B |= (1<<CS11) | (1<<CS10); 
}

void apagar(void){
	//Desconectar el pin PB1
	TCCR1A &= ~(1<<COM1A0);
	
	//Apagar el reloj del Timer1
	TCCR1B &= ~((1<< CS12) | (1<<CS11) | (1<<CS10)); 
}

//Lectura de botones

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

int main(void)
{
	timer1_init();
	botones_init();
	
	int8_t nota_actual= -1;
    /* Replace with your application code */
    while (1) 
    {
		nota_actual = obtener_nota_pulsada();
		
		if(nota_actual !=-1){
			reproducir_nota(NOTAS[nota_actual]);
    }else{
		apagar();
	}
	_delay_ms(10); //Antirrebote
	
	}
	return 0;
}

