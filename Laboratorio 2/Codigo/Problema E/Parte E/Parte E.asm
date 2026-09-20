.INCLUDE "m328pdef.inc"

; Estados:
.EQU CERRADA   = 0
.EQU ABRIENDO  = 1
.EQU ABIERTA   = 2
.EQU CERRANDO  = 3
.EQU SEGURIDAD = 4

; Registros:
.DEF TEMP   = R16                  ; Registro auxiliar
.DEF ESTADO = R20                  ; Guarda el estado actual de la puerta

; Vector de RESET:
.ORG 0x0000
    RJMP INICIO                    ; Al encender/resetear, ir a INICIO


.ORG 0x0006
    RJMP ISR_OBSTACULO             ; Vector de interrupción PCINT0

.ORG 0x0034                        ; El programa principal empieza después de los vectores



; Inicio:
INICIO:

    CLI                            ; Deshabilita interrupciones mientras configuramos	

    LDI TEMP, HIGH(RAMEND)         ; Carga la parte alta del final de la SRAM
    OUT SPH, TEMP                  ; Configura parte alta del Stack Pointer

    LDI TEMP, LOW(RAMEND)          ; Carga la parte baja del final de la SRAM
    OUT SPL, TEMP                  ; Configura parte baja del Stack Pointer


    ; Entradas PORTD:
    CBI DDRD, DDD4                 ; PD4 = entrada, botón ABRIR
    CBI DDRD, DDD5                 ; PD5 = entrada, botón CERRAR
    CBI DDRD, DDD6                 ; PD6 = entrada, sensor puerta ABIERTA
    CBI DDRD, DDD7                 ; PD7 = entrada, sensor puerta CERRADA

    SBI PORTD, PORTD4              ; Activa pull-up interna en PD4
    SBI PORTD, PORTD5              ; Activa pull-up interna en PD5
    SBI PORTD, PORTD6              ; Activa pull-up interna en PD6
    SBI PORTD, PORTD7              ; Activa pull-up interna en PD7


    ; Entrada de obstáculo:
    CBI DDRB, DDB0                 ; PB0 = entrada, sensor de obstáculo
    SBI PORTB, PORTB0              ; Activa pull-up interna en PB0


    ; Salidas PORTB:
    SBI DDRB, DDB1                 ; PB1 = salida, orden motor ABRIR
    SBI DDRB, DDB2                 ; PB2 = salida, orden motor CERRAR
    SBI DDRB, DDB3                 ; PB3 = salida, alarma


    ; Estado inicial de las salidas:
    CBI PORTB, PORTB1              ; Motor ABRIR apagado
    CBI PORTB, PORTB2              ; Motor CERRAR apagado
    CBI PORTB, PORTB3              ; Alarma apagada

    ; Interrupción del sensor de obstáculo:
    LDS TEMP, PCICR                ; Leer registro de control de PCINT
    ORI TEMP, (1<<PCIE0)           ; Habilitar grupo de interrupciones de PORTB
    STS PCICR, TEMP                ; Guardar configuración

    LDS TEMP, PCMSK0               ; Leer máscara de interrupciones de PORTB
    ORI TEMP, (1<<PCINT0)          ; Habilitar PB0 como fuente de interrupción
    STS PCMSK0, TEMP               ; Guardar configuración

    LDI TEMP, (1<<PCIF0)           ; Preparar limpieza de bandera pendiente
    OUT PCIFR, TEMP                ; Limpiar posible interrupción anterior

    LDI ESTADO, CERRADA            ; La puerta comienza en estado CERRADA

    SEI                            ; Habilitar interrupciones globalmente


; Bucle principal:
PRINCIPAL:

    CPI ESTADO, CERRADA            ; ¿Estado = CERRADA?
    BREQ ESTADO_CERRADA            ; Sí ? ir a ESTADO_CERRADA

    CPI ESTADO, ABRIENDO           ; ¿Estado = ABRIENDO?
    BREQ ESTADO_ABRIENDO           ; Sí ? ir a ESTADO_ABRIENDO

    CPI ESTADO, ABIERTA            ; ¿Estado = ABIERTA?
    BREQ ESTADO_ABIERTA            ; Sí ? ir a ESTADO_ABIERTA

    CPI ESTADO, CERRANDO           ; ¿Estado = CERRANDO?
    BREQ ESTADO_CERRANDO           ; Sí ? ir a ESTADO_CERRANDO

    CPI ESTADO, SEGURIDAD          ; ¿Estado = SEGURIDAD?
    BREQ ESTADO_SEGURIDAD          ; Sí ? ir a ESTADO_SEGURIDAD

    RJMP PRINCIPAL                 ; Repetir continuamente


; Estado: CERRADA
ESTADO_CERRADA:

    CBI PORTB, PORTB1              ; Motor ABRIR apagado
    CBI PORTB, PORTB2              ; Motor CERRAR apagado
    CBI PORTB, PORTB3              ; Alarma apagada

    SBIC PIND, PIND4               ; Si PD4 está en 0, omite el RJMP
    RJMP PRINCIPAL                 ; Si no se pulsó ABRIR, seguir esperando

    LDI ESTADO, ABRIENDO           ; Botón ABRIR pulsado ? cambiar estado
    RJMP PRINCIPAL                 ; Volver al control principal


; Estado: ABRIENDO
ESTADO_ABRIENDO:

    SBI PORTB, PORTB1              ; Activar orden de apertura
    CBI PORTB, PORTB2              ; Asegurar orden de cierre apagada
    SBI PORTB, PORTB3              ; Activar alarma

    SBIC PIND, PIND6               ; Si sensor ABIERTA está en 0, omite RJMP
    RJMP PRINCIPAL                 ; Si aún no llegó, continuar abriendo

    CBI PORTB, PORTB1              ; Detener motor
    CBI PORTB, PORTB3              ; Apagar alarma

    LDI ESTADO, ABIERTA            ; Cambiar estado a ABIERTA
    RJMP PRINCIPAL                 ; Volver al control principal


; Estado: ABIERTA
ESTADO_ABIERTA:

    CBI PORTB, PORTB1              ; Motor ABRIR apagado
    CBI PORTB, PORTB2              ; Motor CERRAR apagado
    CBI PORTB, PORTB3              ; Alarma apagada

    SBIC PIND, PIND5               ; Si botón CERRAR está en 0, omite RJMP
    RJMP PRINCIPAL                 ; Si no se pulsó, seguir esperando

    LDI ESTADO, CERRANDO           ; Botón CERRAR pulsado ? cambiar estado
    RJMP PRINCIPAL                 ; Volver al control principal


; Estado: CERRANDO
ESTADO_CERRANDO:

    CBI PORTB, PORTB1              ; Asegurar orden de apertura apagada
    SBI PORTB, PORTB2              ; Activar orden de cierre
    SBI PORTB, PORTB3              ; Activar alarma

    SBIC PIND, PIND7               ; Si sensor CERRADA está en 0, omite RJMP
    RJMP PRINCIPAL                 ; Si aún no llegó, continuar cerrando

    CBI PORTB, PORTB2              ; Detener motor
    CBI PORTB, PORTB3              ; Apagar alarma

    LDI ESTADO, CERRADA            ; Cambiar estado a CERRADA
    RJMP PRINCIPAL                 ; Volver al control principal


; Estado: SEGURIDAD
ESTADO_SEGURIDAD:

    CBI PORTB, PORTB1              ; Detener apertura
    CBI PORTB, PORTB2              ; Detener cierre
    CBI PORTB, PORTB3              ; Apagar alarma


    SBIC PINB, PINB0               ; ¿Ya desapareció el obstáculo?
    RJMP SEGURIDAD_LIBRE           ; Si PB0 = 1, permitir nueva orden

    RJMP PRINCIPAL                 ; Si sigue el obstáculo, permanecer detenido


SEGURIDAD_LIBRE:

    SBIC PIND, PIND4               ; ¿Se pulsó ABRIR?
    RJMP COMPROBAR_CERRAR          ; No ? comprobar botón CERRAR

    LDI ESTADO, ABRIENDO           ; Nueva orden: volver a abrir
    RJMP PRINCIPAL


COMPROBAR_CERRAR:

    SBIC PIND, PIND5               ; ¿Se pulsó CERRAR?
    RJMP PRINCIPAL                 ; No ? permanecer esperando

    LDI ESTADO, CERRANDO           ; Nueva orden: volver a cerrar


    RJMP PRINCIPAL                 ; Por ahora permanece detenido



; Interrupción: obstáculo
ISR_OBSTACULO:

    PUSH TEMP                      ; Guardar R16 antes de modificarlo

    IN TEMP, SREG                  ; Leer registro de estado
    PUSH TEMP                      ; Guardar SREG en el Stack

    SBIC PINB, PINB0               ; Si PB0 = 0 hay obstáculo
    RJMP ISR_SALIR                 ; Si PB0 = 1, fue liberado y salimos

    CPI ESTADO, ABRIENDO           ; ¿La puerta estaba abriendo?
    BREQ ISR_DETENER               ; Sí ? detener

    CPI ESTADO, CERRANDO           ; ¿La puerta estaba cerrando?
    BREQ ISR_DETENER               ; Sí ? detener

    RJMP ISR_SALIR                 ; Si estaba quieta, no hacer nada


ISR_DETENER:

    CBI PORTB, PORTB1              ; Detener motor de apertura
    CBI PORTB, PORTB2              ; Detener motor de cierre
    CBI PORTB, PORTB3              ; Apagar alarma

    LDI ESTADO, SEGURIDAD          ; Pasar al estado de seguridad


ISR_SALIR:

    POP TEMP                       ; Recuperar SREG
    OUT SREG, TEMP                 ; Restaurar registro de estado

    POP TEMP                       ; Recuperar valor original de R16

    RETI                           ; Volver del servicio de interrupción