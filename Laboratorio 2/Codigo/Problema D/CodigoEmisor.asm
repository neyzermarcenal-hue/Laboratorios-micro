.include "m328pdef.inc"

.org 0x0000
    rjmp main

main:
    ;Stack Pointer 
    ldi r16, HIGH(RAMEND)
    out SPH, r16

    ldi r16, LOW(RAMEND)
    out SPL, r16

 ;Configuracion de Pines
    in r16, DDRD
    andi r16, ~((1<<2) | (1<<3) | (1<<4)) ; Configurar PD2..PD4 como entradas
    out DDRD, r16

    in r16, PORTD
    andi r16, ~((1<<2) | (1<<3) | (1<<4)) ; Desactivar pull-ups (asume pull-down externas)
    out PORTD, r16

