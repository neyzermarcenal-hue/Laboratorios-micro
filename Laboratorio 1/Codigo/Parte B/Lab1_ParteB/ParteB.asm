.include "m328pdef.inc"
.org 0x00
.cseg

    ldi r16, 0x3F ; Numero 0
    ldi r17, 0x06 ; Numero 1
    ldi r18, 0x5B ; Numero 2
    ldi r19, 0x4F ; Numero 3
    ldi r20, 0x66 ; Numero 4
    ldi r21, 0x6D ; Numero 5
    ldi r22, 0x7D ; Numero 6
    ldi r23, 0x07 ; Numero 7
    ldi r24, 0x7F ; Numero 8
    ldi r25, 0x6F ; Numero 9

    ldi r26, 0xFF
    out DDRD, r26      ; Puerto D como SALIDA (Display)

    clr r26
    out DDRB, r26      ; Puerto B como ENTRADA (Botones)
    
    ldi r26, 0x07
    out PORTB, r26     ; Activa Pull-Up en PB0, PB1 y PB2

    ; --- Inicializar nuestro contador en r29 ---
    ldi r29, 0         ; r29 sera nuestro contador del 0 al 9
    rcall actualizar_display ; Llamamos a la rutina para que muestre el 0 al inicio