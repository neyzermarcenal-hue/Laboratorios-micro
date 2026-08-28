.include "m328pdef.inc"
.org 0x00
.cseg

inicio:
; Stack Pointer 
    ldi r16, HIGH(RAMEND)
    out SPH, r16
    ldi r16, LOW(RAMEND)
    out SPL, r16

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
    ldi r29, 0         ; r29 será nuestro contador del 0 al 9
    rcall actualizar_display ; Llamamos a la rutina para que muestre el 0 al inicio

loop:
    ;  Lectura de Botones
    sbis PINB, 0       ; Si se presiona PB0 (Incremento)
    rjmp boton_inc

    sbis PINB, 1       ; Si se presiona PB1 (Decremento)
    rjmp boton_dec

    sbis PINB, 2       ; Si se presiona PB2 (Reset)
    rjmp boton_reset

    rjmp loop          ; Volver a revisar si no hay nada presionado

; Botones
boton_inc:
    cpi r29, 9         ; Compara si el contador ya llegó a 9
    breq esperar_soltar_inc ; Si es igual a 9, no incrementa más
    inc r29            ; Incrementa el contador (le suma 1)
    rcall actualizar_display

esperar_soltar_inc:
    sbis PINB, 0       ; Espera a que el botón se suelte (vuelva a 1)
    rjmp esperar_soltar_inc
    rcall retardo      ; Retardo para eliminar rebote al soltar
    rjmp loop

boton_dec:
    cpi r29, 0         ; Compara si el contador ya está en 0
    breq esperar_soltar_dec ; Si es igual a 0, no decrementa más
    dec r29            ; Decrementa el contador (le resta 1)
    rcall actualizar_display

esperar_soltar_dec:
    sbis PINB, 1       ; Espera a que el botón se suelte
    rjmp esperar_soltar_dec
    rcall retardo
    rjmp loop

boton_reset:
    ldi r29, 0         ; Vuelve el contador a 0
    rcall actualizar_display

esperar_soltar_reset:
    sbis PINB, 2       ; Espera a que el botón se suelte
    rjmp esperar_soltar_reset
    rcall retardo
    rjmp loop

; Rutina para mostrar los números en el display
actualizar_display:
    cpi r29, 0
    breq mostrar_0
    cpi r29, 1
    breq mostrar_1
    cpi r29, 2
    breq mostrar_2
    cpi r29, 3
    breq mostrar_3
    cpi r29, 4
    breq mostrar_4
    cpi r29, 5
    breq mostrar_5
    cpi r29, 6
    breq mostrar_6
    cpi r29, 7
    breq mostrar_7
    cpi r29, 8
    breq mostrar_8
    cpi r29, 9
    breq mostrar_9
    ret
;Bloques para enviar el registro correcto al display
mostrar_0: out PORTD, r16 
           ret
mostrar_1: out PORTD, r17 
           ret
mostrar_2: out PORTD, r18 
           ret
mostrar_3: out PORTD, r19 
           ret
mostrar_4: out PORTD, r20 
           ret
mostrar_5: out PORTD, r21 
           ret
mostrar_6: out PORTD, r22 
           ret
mostrar_7: out PORTD, r23 
           ret
mostrar_8: out PORTD, r24 
           ret
mostrar_9: out PORTD, r25 
           ret

;Antirrebote y Retardos 
retardo:
    ldi r27, 20          ; Bucle exterior principal
delay_ext:
    ldi r28, 200         ; Bucle intermedio
delay_mid:
    ldi r30, 250         ; Bucle interior
delay_int:
    dec r30
    brne delay_int
    dec r28
    brne delay_mid
    dec r27
    brne delay_ext
    ret