
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
