.include "m328pdef.inc"

.org 0x0000
    rjmp main

main:
    ldi r16, HIGH(RAMEND)
    out SPH, r16

    ldi r16, LOW(RAMEND)
    out SPL, r16

    ; PC0-PC5 como salidas
    ldi r16, 0x3F
    out DDRC, r16

    ; PB0-PB1 como salidas
    ldi r16, 0x03
    out DDRB, r16

    ; Apagar todos los LEDs al inicio
    clr r16
    out PORTC, r16
    out PORTB, r16

	;Baud rate
    clr r16
    sts UBRR0H, r16

    ldi r16, 103
    sts UBRR0L, r16

    ; Habilitar únicamente la recepción (RXEN0)
    ldi r16, (1 << RXEN0)
    sts UCSR0B, r16

    ; Configurar marco: 8 bits de datos, 1 bit de parada
    ldi r16, (1 << UCSZ01) | (1 << UCSZ00)
    sts UCSR0C, r16

;Bucle Principal
loop_rx:

wait_rx:
    ; Esperar a que el bit RXC0 indique un dato entrante disponible
    lds r17, UCSR0A
    sbrs r17, RXC0
    rjmp wait_rx

    ; Leer el dato recibido desde el registro de datos UDR0
    lds r16, UDR0

    ; Filtrar la máscara: conservar los 3 bits inferiores (0 a 7)
    andi r16, 0x07

	out PORTC, r16

    rjmp loop_rx
