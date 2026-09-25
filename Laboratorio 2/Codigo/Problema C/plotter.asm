.include "m328pdef.inc"

.org 0x0000
    rjmp main


;========================================
; PROGRAMA PRINCIPAL
;========================================

main:

    ldi r16, HIGH(RAMEND)
    out SPH, r16

    ldi r16, LOW(RAMEND)
    out SPL, r16

    ; Configurar PORTD
    ; PD2 = Bajar lápiz
    ; PD3 = Subir lápiz
    ; PD4 = Abajo
    ; PD5 = Arriba
    ; PD6 = Izquierda
    ; PD7 = Derecha

    ldi r16, 0b11111100
    out DDRD, r16


    ; Todas las salidas comienzan apagadas

    clr r16
    out PORTD, r16


;========================================
; BUCLE PRINCIPAL
;========================================

loop:

    rjmp loop
