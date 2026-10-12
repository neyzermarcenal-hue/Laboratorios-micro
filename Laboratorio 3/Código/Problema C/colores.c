#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>

void configurar_ADC(void)
{
    ADMUX = (1 << REFS0);
    ADCSRA = (1 << ADEN) |
             (1 << ADPS2) |
             (1 << ADPS1) |
             (1 << ADPS0);
}

unsigned int leer_ADC(void)
{
    ADCSRA |= (1 << ADSC);

    while (ADCSRA & (1 << ADSC))
    {
    }

    return ADC;
}

void configurar_UART(void)
{
    UBRR0H = 0;
    UBRR0L = 103;

    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void enviar_caracter(char dato)
{
    while (!(UCSR0A & (1 << UDRE0)))
    {
    }

    UDR0 = dato;
}

void enviar_numero(unsigned int numero)
{
    char digitos[5];
    int i = 0;

    if (numero == 0)
    {
        enviar_caracter('0');
    }

    while (numero > 0)
    {
        digitos[i] = (numero % 10) + '0';
        numero = numero / 10;
        i++;
    }

    while (i > 0)
    {
        i--;
        enviar_caracter(digitos[i]);
    }
}

int main(void)
{
    unsigned int lectura;

    configurar_ADC();
    configurar_UART();

    while (1)
    {
        lectura = leer_ADC();

        enviar_caracter('A');
        enviar_caracter('D');
        enviar_caracter('C');
        enviar_caracter(':');
        enviar_caracter(' ');

        enviar_numero(lectura);

        enviar_caracter('\r');
        enviar_caracter('\n');

        _delay_ms(500);
    }
}
