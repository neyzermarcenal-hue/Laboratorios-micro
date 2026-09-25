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

    rcall dibujar_triangulo

    rjmp volver_menu


opcion_2:

    rcall dibujar_circulo

    rjmp volver_menu


opcion_3:

    rcall dibujar_pentagrama

    rjmp volver_menu


opcion_4:

    rcall dibujar_libre

    rjmp volver_menu


opcion_P:

    rcall dibujar_gengar

    rjmp volver_menu


opcion_T:

    rcall dibujar_todas

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
; MOVIMIENTOS DIAGONALES
;==================================================

diagonal_abajo_derecha:

    sbi PORTD, PD7
    rcall delay_paso
    cbi PORTD, PD7

    sbi PORTD, PD4
    rcall delay_paso
    cbi PORTD, PD4

    ret


diagonal_abajo_izquierda:

    sbi PORTD, PD6
    rcall delay_paso
    cbi PORTD, PD6

    sbi PORTD, PD4
    rcall delay_paso
    cbi PORTD, PD4

    ret


diagonal_arriba_derecha:

    sbi PORTD, PD7
    rcall delay_paso
    cbi PORTD, PD7

    sbi PORTD, PD5
    rcall delay_paso
    cbi PORTD, PD5

    ret


diagonal_arriba_izquierda:

    sbi PORTD, PD6
    rcall delay_paso
    cbi PORTD, PD6

    sbi PORTD, PD5
    rcall delay_paso
    cbi PORTD, PD5

    ret

;==================================================
; MOVIMIENTOS DE POSICIONAMIENTO
; r21 = cantidad de pasos
;==================================================

pasos_abajo:

pasos_abajo_loop:

    sbi PORTD, PD4
    rcall delay_paso
    cbi PORTD, PD4

    dec r21
    brne pasos_abajo_loop

    ret


pasos_arriba:

pasos_arriba_loop:

    sbi PORTD, PD5
    rcall delay_paso
    cbi PORTD, PD5

    dec r21
    brne pasos_arriba_loop

    ret


pasos_izquierda:

pasos_izquierda_loop:

    sbi PORTD, PD6
    rcall delay_paso
    cbi PORTD, PD6

    dec r21
    brne pasos_izquierda_loop

    ret


pasos_derecha:

pasos_derecha_loop:

    sbi PORTD, PD7
    rcall delay_paso
    cbi PORTD, PD7

    dec r21
    brne pasos_derecha_loop

    ret

;==================================================
; FIGURA 1 - TRIANGULO
;==================================================

dibujar_triangulo:

    ; Bajar lapiz
    rcall lapiz_abajo


    ; Primer lado diagonal abajo-izquierda

    ldi r21, 20

triangulo_lado1:

    rcall diagonal_abajo_izquierda

    dec r21
    brne triangulo_lado1


    ; Base hacia la derecha

    ldi r21, 40

triangulo_base:

    sbi PORTD, PD7
    rcall delay_paso
    cbi PORTD, PD7

    dec r21
    brne triangulo_base


    ; Segundo lado diagonal arriba-izquierda

    ldi r21, 20

triangulo_lado2:

    rcall diagonal_arriba_izquierda

    dec r21
    brne triangulo_lado2


    ; Subir lapiz

    rcall lapiz_arriba

    ret

;==================================================
; FIGURA 2 - CIRCULO
;==================================================

dibujar_circulo:

    ; Bajar lapiz
    rcall lapiz_abajo


    ;----------------------------------------------
    ; Parte superior hacia la izquierda
    ;----------------------------------------------

    ldi r21, 6

circulo_1:

    sbi PORTD, PD6
    rcall delay_paso
    cbi PORTD, PD6

    dec r21
    brne circulo_1


    ;----------------------------------------------
    ; Curva superior izquierda
    ;----------------------------------------------

    ldi r21, 8

circulo_2:

    rcall diagonal_abajo_izquierda

    dec r21
    brne circulo_2


    ;----------------------------------------------
    ; Lado izquierdo
    ;----------------------------------------------

    ldi r21, 6

circulo_3:

    sbi PORTD, PD4
    rcall delay_paso
    cbi PORTD, PD4

    dec r21
    brne circulo_3


    ;----------------------------------------------
    ; Curva inferior izquierda
    ;----------------------------------------------

    ldi r21, 8

circulo_4:

    rcall diagonal_abajo_derecha

    dec r21
    brne circulo_4


    ;----------------------------------------------
    ; Parte inferior
    ;----------------------------------------------

    ldi r21, 6

circulo_5:

    sbi PORTD, PD7
    rcall delay_paso
    cbi PORTD, PD7

    dec r21
    brne circulo_5


    ;----------------------------------------------
    ; Curva inferior derecha
    ;----------------------------------------------

    ldi r21, 8

circulo_6:

    rcall diagonal_arriba_derecha

    dec r21
    brne circulo_6


    ;----------------------------------------------
    ; Lado derecho
    ;----------------------------------------------

    ldi r21, 6

circulo_7:

    sbi PORTD, PD5
    rcall delay_paso
    cbi PORTD, PD5

    dec r21
    brne circulo_7


    ;----------------------------------------------
    ; Curva superior derecha
    ;----------------------------------------------

    ldi r21, 8

circulo_8:

    rcall diagonal_arriba_izquierda

    dec r21
    brne circulo_8


    ; Subir lapiz

    rcall lapiz_arriba

    ret

;==================================================
; FIGURA 3 - PENTAGRAMA
;==================================================

dibujar_pentagrama:

    ; Bajar lapiz

    rcall lapiz_abajo


    ;----------------------------------------------
    ; Linea 1
    ; Desde arriba hacia abajo-izquierda
    ;----------------------------------------------

    ldi r21, 20

pentagrama_1:

    rcall diagonal_abajo_izquierda

    dec r21
    brne pentagrama_1


    ;----------------------------------------------
    ; Linea 2
    ; Hacia la derecha
    ;----------------------------------------------

    ldi r21, 35

pentagrama_2:

    sbi PORTD, PD7
    rcall delay_paso
    cbi PORTD, PD7

    dec r21
    brne pentagrama_2


    ;----------------------------------------------
    ; Linea 3
    ; Arriba-izquierda
    ;----------------------------------------------

    ldi r21, 20

pentagrama_3:

    rcall diagonal_arriba_izquierda

    dec r21
    brne pentagrama_3


    ;----------------------------------------------
    ; Linea 4
    ; Abajo-izquierda
    ; Cruza el centro
    ;----------------------------------------------

    ldi r21, 28

pentagrama_4:

    rcall diagonal_abajo_izquierda

    dec r21
    brne pentagrama_4


    ;----------------------------------------------
    ; Linea 5
    ; Arriba-derecha
    ; Regresa hacia el inicio
    ;----------------------------------------------

    ldi r21, 28

pentagrama_5:

    rcall diagonal_arriba_derecha

    dec r21
    brne pentagrama_5


    ; Subir lapiz

    rcall lapiz_arriba

    ret


;==================================================
; FIGURA 4 - FIGURA LIBRE: CORAZON
;==================================================

dibujar_libre:

    ; Bajar lapiz
    rcall lapiz_abajo


    ;----------------------------------------------
    ; Desde el centro superior hacia arriba-izquierda
    ;----------------------------------------------

    ldi r21, 8

corazon_1:

    rcall diagonal_arriba_izquierda

    dec r21
    brne corazon_1


    ;----------------------------------------------
    ; Parte superior izquierda
    ;----------------------------------------------

    ldi r21, 8

corazon_2:

    rcall diagonal_abajo_izquierda

    dec r21
    brne corazon_2


    ;----------------------------------------------
    ; Lado izquierdo hacia la punta inferior
    ;----------------------------------------------

    ldi r21, 16

corazon_3:

    rcall diagonal_abajo_derecha

    dec r21
    brne corazon_3


    ;----------------------------------------------
    ; Desde la punta inferior hacia lado derecho
    ;----------------------------------------------

    ldi r21, 16

corazon_4:

    rcall diagonal_arriba_derecha

    dec r21
    brne corazon_4


    ;----------------------------------------------
    ; Parte superior derecha
    ;----------------------------------------------

    ldi r21, 8

corazon_5:

    rcall diagonal_arriba_izquierda

    dec r21
    brne corazon_5


    ;----------------------------------------------
    ; Volver al centro superior
    ;----------------------------------------------

    ldi r21, 8

corazon_6:

    rcall diagonal_abajo_izquierda

    dec r21
    brne corazon_6


    ; Subir lapiz
    rcall lapiz_arriba

    ret

;==================================================
; POKEMON - GENGAR
;==================================================

dibujar_gengar:

    ;----------------------------------------------
    ; SILUETA
    ; Comenzamos en la parte superior central
    ;----------------------------------------------

    rcall lapiz_abajo


    ; Hacia oreja izquierda

    ldi r21, 10

gengar_silueta_1:

    rcall diagonal_arriba_izquierda

    dec r21
    brne gengar_silueta_1


    ; Punta de oreja izquierda hacia afuera

    ldi r21, 8

gengar_silueta_2:

    rcall diagonal_abajo_izquierda

    dec r21
    brne gengar_silueta_2


    ; Lado izquierdo de la cabeza

    ldi r21, 10

gengar_silueta_3:

    rcall diagonal_abajo_izquierda

    dec r21
    brne gengar_silueta_3


    ; Lateral izquierdo del cuerpo

    ldi r21, 12

gengar_silueta_4:

    sbi PORTD, PD4
    rcall delay_paso
    cbi PORTD, PD4

    dec r21
    brne gengar_silueta_4


    ; Parte inferior izquierda

    ldi r21, 10

gengar_silueta_5:

    rcall diagonal_abajo_derecha

    dec r21
    brne gengar_silueta_5


    ; Primera pata / punta inferior

    ldi r21, 6

gengar_silueta_6:

    rcall diagonal_arriba_derecha

    dec r21
    brne gengar_silueta_6


    ldi r21, 6

gengar_silueta_7:

    rcall diagonal_abajo_derecha

    dec r21
    brne gengar_silueta_7


    ; Centro inferior

    ldi r21, 12

gengar_silueta_8:

    sbi PORTD, PD7
    rcall delay_paso
    cbi PORTD, PD7

    dec r21
    brne gengar_silueta_8


    ; Segunda punta inferior

    ldi r21, 6

gengar_silueta_9:

    rcall diagonal_arriba_derecha

    dec r21
    brne gengar_silueta_9


    ldi r21, 6

gengar_silueta_10:

    rcall diagonal_abajo_derecha

    dec r21
    brne gengar_silueta_10


    ; Parte inferior derecha

    ldi r21, 10

gengar_silueta_11:

    rcall diagonal_arriba_derecha

    dec r21
    brne gengar_silueta_11


    ; Lateral derecho

    ldi r21, 12

gengar_silueta_12:

    sbi PORTD, PD5
    rcall delay_paso
    cbi PORTD, PD5

    dec r21
    brne gengar_silueta_12


    ; Subir hacia oreja derecha

    ldi r21, 10

gengar_silueta_13:

    rcall diagonal_arriba_izquierda

    dec r21
    brne gengar_silueta_13


    ; Oreja derecha

    ldi r21, 8

gengar_silueta_14:

    rcall diagonal_arriba_izquierda

    dec r21
    brne gengar_silueta_14


    ; Regresar al centro superior

    ldi r21, 10

gengar_silueta_15:

    rcall diagonal_abajo_izquierda

    dec r21
    brne gengar_silueta_15


    rcall lapiz_arriba

    ret

;==================================================
; OJO IZQUIERDO DE GENGAR
;==================================================

gengar_ojo_izquierdo:

    ; El lapiz debe llegar levantado

    ; Bajar lapiz para comenzar el ojo
    rcall lapiz_abajo


    ; Parte superior inclinada
    ; Abajo-derecha

    ldi r21, 8

ojo_izq_1:

    rcall diagonal_abajo_derecha

    dec r21
    brne ojo_izq_1


    ; Parte inferior hacia la izquierda

    ldi r21, 8

ojo_izq_2:

    rcall diagonal_abajo_izquierda

    dec r21
    brne ojo_izq_2


    ; Cerrar el ojo hacia arriba

    ldi r21, 8
    rcall pasos_arriba


    ; Subir lapiz

    rcall lapiz_arriba

    ret

;==================================================
; OJO DERECHO DE GENGAR
;==================================================

gengar_ojo_derecho:

    rcall lapiz_abajo


    ; Parte superior inclinada
    ; Abajo-izquierda

    ldi r21, 8

ojo_der_1:

    rcall diagonal_abajo_izquierda

    dec r21
    brne ojo_der_1


    ; Parte inferior hacia la derecha

    ldi r21, 8

ojo_der_2:

    rcall diagonal_abajo_derecha

    dec r21
    brne ojo_der_2


    ; Cerrar ojo

    ldi r21, 8
    rcall pasos_arriba


    rcall lapiz_arriba

    ret

;==================================================
; BOCA DE GENGAR
;==================================================

gengar_boca:

    rcall lapiz_abajo


    ; Parte superior de la boca

    ldi r21, 24
    rcall pasos_derecha


    ; Esquina derecha hacia abajo

    ldi r21, 5

boca_1:

    rcall diagonal_abajo_izquierda

    dec r21
    brne boca_1


    ; Parte inferior hacia la izquierda

    ldi r21, 14
    rcall pasos_izquierda


    ; Cerrar lado izquierdo

    ldi r21, 5

boca_2:

    rcall diagonal_arriba_izquierda

    dec r21
    brne boca_2


    rcall lapiz_arriba

    ;==============================================
    ; POSICIONAR EN OJO IZQUIERDO
    ;==============================================

    ldi r21, 18
    rcall pasos_izquierda

    ldi r21, 15
    rcall pasos_abajo

    rcall gengar_ojo_izquierdo


    ;==============================================
    ; POSICIONAR EN OJO DERECHO
    ;==============================================

    ldi r21, 18
    rcall pasos_derecha

    rcall gengar_ojo_derecho


    ;==============================================
    ; POSICIONAR EN LA BOCA
    ;==============================================

    ldi r21, 20
    rcall pasos_izquierda

    ldi r21, 12
    rcall pasos_abajo

    rcall gengar_boca


    ret

;==================================================
; OPCION T - DIBUJAR TODAS LAS FIGURAS
;==================================================

dibujar_todas:

    ;----------------------------------------------
    ; FIGURA 1 - TRIANGULO
    ;----------------------------------------------

    rcall dibujar_triangulo

    ; Mover a nueva posicion con lapiz arriba

    ldi r21, 15
    rcall pasos_derecha


    ;----------------------------------------------
    ; FIGURA 2 - CIRCULO
    ;----------------------------------------------

    rcall dibujar_circulo

    ; Mover a nueva posicion

    ldi r21, 15
    rcall pasos_derecha


    ;----------------------------------------------
    ; FIGURA 3 - PENTAGRAMA
    ;----------------------------------------------

    rcall dibujar_pentagrama

    ; Mover hacia abajo para segunda fila

    ldi r21, 40
    rcall pasos_izquierda

    ldi r21, 30
    rcall pasos_abajo


    ;----------------------------------------------
    ; FIGURA 4 - FIGURA LIBRE
    ;----------------------------------------------

    rcall dibujar_libre

    ; Mover hacia la derecha

    ldi r21, 25
    rcall pasos_derecha


    ;----------------------------------------------
    ; POKEMON - GENGAR
    ;----------------------------------------------

    rcall dibujar_gengar


    ;----------------------------------------------
    ; FIN
    ;----------------------------------------------

    rcall lapiz_arriba

    ret

;==================================================
; RETARDO PARA PASOS PEQUEÑOS
;==================================================

delay_paso:

    ldi r18, 2

delay_paso_ext:

    ldi r19, 255

delay_paso_int:

    dec r19
    brne delay_paso_int

    dec r18
    brne delay_paso_ext

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
