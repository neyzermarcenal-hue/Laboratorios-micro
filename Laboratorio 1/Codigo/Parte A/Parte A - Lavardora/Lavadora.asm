.include "m328pdef.inc"


; Registros que vamos a usar:

.def temp  = R16        ; Registro temporal para hacer configuraciones
.def carga = R17        ; Guarda la carga seleccionada:
                        ; 0 = ligera
                        ; 1 = media
                        ; 2 = pesada
.def segundos = R18     ; Guarda cuántos segundos tenemos que esperar
.def ciclos   = R19     ; Cuenta las 5 repeticiones del lavado


.org 0x0000
    RJMP INICIO         ; Al arrancar el micro, va al inicio del programa



INICIO:


    ; Configuro el Stack Pointer:

    LDI temp, HIGH(RAMEND)    ; Carga la parte alta de la última dirección SRAM
    OUT SPH, temp             ; La guarda en la parte alta del Stack Pointer

    LDI temp, LOW(RAMEND)     ; Carga la parte baja de la última dirección SRAM
    OUT SPL, temp             ; La guarda en la parte baja del Stack Pointer


    ; Configuramos Timer1 para usarlo en los retardos

    CLR temp
    STS TCCR1A, temp             ; Timer1 en funcionamiento normal

    LDI temp, (1<<CS12)          ; Prescaler de 256
    STS TCCR1B, temp             ; Arranca Timer1 con ese prescaler

    CLR temp
    STS TIMSK1, temp             ; No usamos interrupciones del Timer1


        /* Configuro las salidas del PORTB
          PB0 = listo
          PB1 = lavado
          PB2 = centrifugado
          PB3 = secado
          PB4 = fin
          PB5 = carga ligera */


    LDI temp, 0b00111111      ; Pongo PB0 hasta PB5 como salidas
    OUT DDRB, temp            ; Guardo esa configuración en DDRB


        /* Configuro las salidas del PORTC
           PC0 = carga media
           PC1 = carga pesada
           PC2 = motor derecha
           PC3 = motor izquierda */


    LDI temp, 0b00001111      ; PC0 hasta PC3 van a ser salidas
    OUT DDRC, temp            ; GuardO la configuración en DDRC


        /* Entradas del sistema
           PD2 = START
           PD3 = selección de carga
           PD4 = sensor de puerta
           PD5 = sensor de nivel */


    CLR temp                   ; temp = 0
    OUT DDRD, temp             ; DejO PORTD como entrada


    ; Al comenzar apago todas las salidas

    CLR temp
    OUT PORTB, temp            ; Apaga las salidas del PORTB
    OUT PORTC, temp            ; Apaga las salidas del PORTC


    ; Activo las resistencias pull-up de PD2 a PD5

    LDI temp, 0b00111100      ; Pongo en 1 PD2, PD3, PD4 y PD5
    OUT PORTD, temp            ; Activa las pull-up internas


    ; La lavadora empieza con carga ligera

    CLR carga                  ; carga = 0, entonces empieza en ligera

    SBI PORTB, PB0             ; Enciende LED de listo
    SBI PORTB, PB5             ; Enciende LED de carga ligera



LISTO:

    SBIS PIND, PD3             ; Si PD3 está en 1, no se presionó CARGA
    RJMP CAMBIAR_CARGA         ; Si está en 0, cambia la carga

    RJMP LISTO                 ; Si no se presionó nada, sigue esperando



PREPARAR_CENTRIFUGADO:

    CBI PORTB, PB1             ; Apago el LED de lavado
    SBI PORTB, PB2             ; Enciendo el LED de centrifugado

    SBI PORTC, PC2             ; Enciendo el LED que representa el motor

    MOV segundos, carga        ; Copio la carga: 0, 1 o 2
    LSL segundos               ; Queda 0, 2 o 4
    ADD segundos, carga        ; Queda 0, 3 o 6

    LDI temp, 15
    ADD segundos, temp         ; Queda 15, 18 o 21 segundos

    RCALL RETARDO_SEGUNDOS     ; Mantiene el centrifugado ese tiempo

    CBI PORTC, PC2             ; Apago el motor
    CBI PORTB, PB2             ; Apago LED de centrifugado

    RJMP PREPARAR_SECADO



PREPARAR_SECADO:

    SBI PORTB, PB3             ; Enciendo el LED de secado

    RJMP SECADO_DERECHA


SECADO_DERECHA:

    SBI PORTC, PC2             ; Motor hacia la derecha
    CBI PORTC, PC3             ; Me aseguro de apagar el de izquierda

    MOV segundos, carga
    LSL segundos               ; 0, 2 o 4

    LDI temp, 5
    ADD segundos, temp         ; 5, 7 o 9 segundos

    RCALL RETARDO_SEGUNDOS

    CBI PORTC, PC2             ; Apago motor derecha

    RJMP SECADO_PAUSA


SECADO_PAUSA:

    MOV segundos, carga
    LSL segundos               ; 0, 2 o 4

    LDI temp, 3
    ADD segundos, temp         ; 3, 5 o 7 segundos

    RCALL RETARDO_SEGUNDOS     ; Espera con los motores apagados

    RJMP SECADO_IZQUIERDA


SECADO_IZQUIERDA:

    SBI PORTC, PC3             ; Motor hacia la izquierda
    CBI PORTC, PC2             ; Me aseguro de apagar el de derecha

    MOV segundos, carga
    LSL segundos               ; 0, 2 o 4

    LDI temp, 5
    ADD segundos, temp         ; 5, 7 o 9 segundos

    RCALL RETARDO_SEGUNDOS

    CBI PORTC, PC3             ; Apago motor izquierda
    CBI PORTB, PB3             ; Apago LED de secado

    RJMP FIN



FIN:

    SBI PORTB, PB4             ; Enciendo el LED de fin

    RJMP FIN                   ; El proceso queda terminado



CAMBIAR_CARGA:

    ; Esperamos a que se suelte el botón antes de seguir

ESPERA_CARGA:

    SBIS PIND, PD3             ; Revisa si el botón ya volvió a 1
    RJMP ESPERA_CARGA          ; Si sigue apretado, se queda esperando

    INC carga                  ; Pasa a la siguiente carga / Incrementa 1

    CPI carga, 3               ; Compara con 3
    BRNE ACTUALIZAR_CARGA      ; Si no llegó a 3, seguimos normalmente

    CLR carga                  ; Si llegó a 3, vuelve a 0 = ligera



ACTUALIZAR_CARGA:

    CBI PORTB, PB5             ; Apaga LED de carga ligera
    CBI PORTC, PC0             ; Apaga LED de carga media
    CBI PORTC, PC1             ; Apaga LED de carga pesada

    CPI carga, 0               ; ¿La carga es ligera?
    BREQ MOSTRAR_LIGERA

    CPI carga, 1               ; ¿La carga es media?
    BREQ MOSTRAR_MEDIA

    ; Si no es 0 ni 1, entonces es pesada

MOSTRAR_PESADA:

    SBI PORTC, PC1             ; Enciende LED de carga pesada
    RJMP LISTO


MOSTRAR_MEDIA:

    SBI PORTC, PC0             ; Enciende LED de carga media
    RJMP LISTO


MOSTRAR_LIGERA:

    SBI PORTB, PB5             ; Enciende LED de carga ligera
    RJMP LISTO



RETARDO_SEGUNDOS:

    RCALL RETARDO_1S           ; Espera un segundo

    DEC segundos               ; Resta un segundo del total
    BRNE RETARDO_SEGUNDOS      ; Si todavía quedan segundos, repite

    RET                        ; Si llegó a 0, vuelve al programa


RETARDO_1S:

    SBI TIFR1, TOV1            ; Limpia la bandera de desbordamiento anterior

    LDI temp, HIGH(3036)       ; Parte alta del valor inicial del Timer1
    STS TCNT1H, temp

    LDI temp, LOW(3036)        ; Parte baja del valor inicial
    STS TCNT1L, temp


ESPERA_TIMER1:

    SBIS TIFR1, TOV1           ; ¿Timer1 ya llegó al desbordamiento?
    RJMP ESPERA_TIMER1         ; No: sigue esperando

    RET                        ; Sí: pasó aproximadamente 1 segundo