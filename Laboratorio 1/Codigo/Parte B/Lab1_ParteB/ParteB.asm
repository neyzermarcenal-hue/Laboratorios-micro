;Antirreobte y retardos
retardo:
    ldi r27, 255
delay_ext:
    ldi r28, 255
delay_int:
    dec r28
    brne delay_int
    dec r27
    brne delay_ext
    ret