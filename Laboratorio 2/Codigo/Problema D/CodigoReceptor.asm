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

    ;DECODIFICADOR 3 A 8 
    ldi r17, 1
    mov r18, r16

    ; Si el valor recibido es 0, no hay desplazamientos que realizar
    cpi r18, 0
    breq mapear_salida

shift_loop:
    lsl r17
    dec r18
    brne shift_loop

;Selecciona y activa el puerto a utilizar
mapear_salida:
    cpi r16, 6
    brlo enviar_portc

; Caso A: Valores 6 o 7 (LED 6 -> D8, LED 7 -> D9)
enviar_portb:
    clr r19
    out PORTC, r19

    lsr r17
    lsr r17
    lsr r17
    lsr r17
    lsr r17
    lsr r17

    out PORTB, r17
    rjmp loop_rx

; Caso B: Valores 0 a 5 (LEDs A0 a A5)
enviar_portc:
    clr r19
    out PORTB, r19

    out PORTC, r17
    rjmp loop_rx
