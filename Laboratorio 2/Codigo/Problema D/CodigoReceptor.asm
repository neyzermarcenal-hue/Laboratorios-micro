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
