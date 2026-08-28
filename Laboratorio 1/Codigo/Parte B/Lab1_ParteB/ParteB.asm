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