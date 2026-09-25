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

    rcall USART_INIT
    rcall MOSTRAR_MENU

    rjmp loop

;==================================================
; CONFIGURACION UART - 9600 BAUDIOS
;==================================================

USART_INIT:

    ; 9600 baudios -> UBRR = 103
    ldi r16, 0
    sts UBRR0H, r16

    ldi r16, 103
    sts UBRR0L, r16

    ; Habilitar transmision y recepcion
    ldi r16, (1<<RXEN0) | (1<<TXEN0)
    sts UCSR0B, r16

    ; 8 bits de datos, 1 bit de parada, sin paridad
    ldi r16, (1<<UCSZ01) | (1<<UCSZ00)
    sts UCSR0C, r16

    ret


;========================================
; BUCLE PRINCIPAL
;========================================

loop:

    ; Esperar opcion del usuario
    rcall USART_RX

    ; Comparar caracter recibido

    cpi r16, '1'
    breq opcion_1

    cpi r16, '2'
    breq opcion_2

    cpi r16, '3'
    breq opcion_3

    cpi r16, '4'
    breq opcion_4

    cpi r16, 'P'
    breq opcion_P

    cpi r16, 'p'
    breq opcion_P

    cpi r16, 'T'
    breq opcion_T

    cpi r16, 't'
    breq opcion_T

    ; Si recibe otro caracter vuelve a esperar

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
; OPCIONES DEL MENU
;==================================================

opcion_1:

    ; dibujar triangulo

    rjmp volver_menu


opcion_2:

    ; dibujar circulo

    rjmp volver_menu


opcion_3:

    ; dibujar pentagrama

    rjmp volver_menu


opcion_4:

    ; dibujar figura libre

    rjmp volver_menu


opcion_P:

    ; dibujar Gengar

    rjmp volver_menu


opcion_T:

    ; dibujar todas las figuras

    rjmp volver_menu


;==================================================
; VOLVER AL MENU
;==================================================

volver_menu:

    rcall MOSTRAR_MENU
    rjmp loop


;==================================================
; TRANSMITIR CARACTER POR UART
;==================================================

USART_TX:

esperar_tx:

    lds r17, UCSR0A

    sbrs r17, UDRE0
    rjmp esperar_tx

    sts UDR0, r16

    ret


;==================================================
; RECIBIR CARACTER POR UART
;
; r16 = caracter recibido
;==================================================

USART_RX:

esperar_rx:

    lds r17, UCSR0A

    sbrs r17, RXC0
    rjmp esperar_rx

    lds r16, UDR0

    ret


;==================================================
; MOSTRAR MENU
;==================================================

MOSTRAR_MENU:

    ; Cargar direccion del menu en Z

    ldi ZH, HIGH(MENU<<1)
    ldi ZL, LOW(MENU<<1)

menu_loop:

    ; Leer caracter desde memoria Flash

    lpm r16, Z+

    ; Si encuentra 0 termina el mensaje

    cpi r16, 0
    breq menu_fin

    ; Enviar caracter

    rcall USART_TX

    rjmp menu_loop

menu_fin:

    ret

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

;==================================================
; MENU GUARDADO EN MEMORIA FLASH
;==================================================

MENU:

.db 13,10
.db "PLOTTER UTEC",13,10
.db "1 - Triangulo",13,10
.db "2 - Circulo",13,10
.db "3 - Pentagrama",13,10
.db "4 - Figura libre",13,10
.db "P - Gengar",13,10
.db "T - Todas",13,10
.db "Seleccione: ",0
