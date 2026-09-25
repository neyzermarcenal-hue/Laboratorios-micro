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


;==================================================
; CONTROL DEL LAPIZ
;==================================================

lapiz_abajo:

    sbi PORTD, PD2       ; Activa bajar solenoide
    rcall delay_rele
    cbi PORTD, PD2       ; Desactiva salida

    ret


lapiz_arriba:

    sbi PORTD, PD3       ; Activa subir solenoide
    rcall delay_rele
    cbi PORTD, PD3       ; Desactiva salida

    ret


;==================================================
; MOVIMIENTOS
;==================================================

mover_abajo:

    sbi PORTD, PD4
    rcall delay_movimiento
    cbi PORTD, PD4

    ret


mover_arriba:

    sbi PORTD, PD5
    rcall delay_movimiento
    cbi PORTD, PD5

    ret


mover_izquierda:

    sbi PORTD, PD6
    rcall delay_movimiento
    cbi PORTD, PD6

    ret


mover_derecha:

    sbi PORTD, PD7
    rcall delay_movimiento
    cbi PORTD, PD7

    ret


;==================================================
; RETARDO PARA MOVIMIENTO
;==================================================

delay_movimiento:

    ldi r18, 20

delay_mov_ext:

    ldi r19, 255

delay_mov_med:

    ldi r20, 255

delay_mov_int:

    dec r20
    brne delay_mov_int

    dec r19
    brne delay_mov_med

    dec r18
    brne delay_mov_ext

    ret


;==================================================
; RETARDO DEL RELE / SOLENOIDE
;==================================================

delay_rele:

    ldi r18, 10

delay_rele_ext:

    ldi r19, 255

delay_rele_med:

    ldi r20, 255

delay_rele_int:

    dec r20
    brne delay_rele_int

    dec r19
    brne delay_rele_med

    dec r18
    brne delay_rele_ext

    ret
