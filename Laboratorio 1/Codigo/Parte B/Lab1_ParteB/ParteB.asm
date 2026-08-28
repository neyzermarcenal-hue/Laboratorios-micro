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