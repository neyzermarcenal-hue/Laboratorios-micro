.include "m328pdef.inc"

.org 0x0000
    rjmp main

;Direcciones para matriz 

.equ G_FIN = 0
.equ G_DER = 1
.equ G_IZQ = 2
.equ G_ARR = 3
.equ G_ABA = 4
.equ G_AD  = 5 ; arriba-derecha
.equ G_AI  = 6 ; arriba-izquierda
.equ G_BD  = 7 ; abajo-derecha
.equ G_BI  = 8 ; abajo-izquierda

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

    ldi r21, 60

triangulo_lado1:

    rcall diagonal_abajo_izquierda

    dec r21
    brne triangulo_lado1


    ; Base hacia la derecha

    ldi r21, 120

triangulo_base:

    sbi PORTD, PD7
    rcall delay_paso
    cbi PORTD, PD7

    dec r21
    brne triangulo_base


    ; Segundo lado diagonal arriba-izquierda

    ldi r21, 60

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

    rcall lapiz_abajo

    ; 1 - PARTE SUPERIOR
    ldi r21, 20
circulo_1:
    sbi PORTD, PD6
    rcall delay_paso
    cbi PORTD, PD6
    dec r21
    brne circulo_1

    ; 2 - CURVA SUPERIOR IZQUIERDA SUAVE
    ldi r21, 10
circulo_2:
    sbi PORTD, PD6
    rcall delay_paso
    cbi PORTD, PD6
    rcall diagonal_abajo_izquierda
    dec r21
    brne circulo_2

    ; 3 - CURVA IZQUIERDA MAS VERTICAL
    ldi r21, 10
circulo_3:
    rcall diagonal_abajo_izquierda
    sbi PORTD, PD4
    rcall delay_paso
    cbi PORTD, PD4
    dec r21
    brne circulo_3

    ; 4 - LADO IZQUIERDO
    ldi r21, 20
circulo_4:
    sbi PORTD, PD4
    rcall delay_paso
    cbi PORTD, PD4
    dec r21
    brne circulo_4

    ; 5 - CURVA INFERIOR IZQUIERDA
    ldi r21, 10
circulo_5:
    sbi PORTD, PD4
    rcall delay_paso
    cbi PORTD, PD4
    rcall diagonal_abajo_derecha
    dec r21
    brne circulo_5

    ; 6 - ENTRANDO A LA PARTE INFERIOR
    ldi r21, 10
circulo_6:
    rcall diagonal_abajo_derecha
    sbi PORTD, PD7
    rcall delay_paso
    cbi PORTD, PD7
    dec r21
    brne circulo_6

    ; 7 - PARTE INFERIOR
    ldi r21, 20
circulo_7:
    sbi PORTD, PD7
    rcall delay_paso
    cbi PORTD, PD7
    dec r21
    brne circulo_7

    ; 8 - CURVA INFERIOR DERECHA SUAVE
    ldi r21, 10
circulo_8:
    sbi PORTD, PD7
    rcall delay_paso
    cbi PORTD, PD7
    rcall diagonal_arriba_derecha
    dec r21
    brne circulo_8

    ; 9 - CURVA DERECHA MAS VERTICAL
    ldi r21, 10
circulo_9:
    rcall diagonal_arriba_derecha
    sbi PORTD, PD5
    rcall delay_paso
    cbi PORTD, PD5
    dec r21
    brne circulo_9

    ; 10 - LADO DERECHO
    ldi r21, 20
circulo_10:
    sbi PORTD, PD5
    rcall delay_paso
    cbi PORTD, PD5
    dec r21
    brne circulo_10

    ; 11 - CURVA SUPERIOR DERECHA
    ldi r21, 10
circulo_11:
    sbi PORTD, PD5
    rcall delay_paso
    cbi PORTD, PD5
    rcall diagonal_arriba_izquierda
    dec r21
    brne circulo_11

    ; 12 - CERRAR CURVA SUPERIOR
    ldi r21, 10
circulo_12:
    rcall diagonal_arriba_izquierda
    sbi PORTD, PD6
    rcall delay_paso
    cbi PORTD, PD6
    dec r21
    brne circulo_12

    rcall lapiz_arriba
    ret

;==================================================
; FIGURA 3 - PENTAGRAMA
;==================================================

dibujar_pentagrama:

    ; Bajar lapiz

    rcall lapiz_abajo
    ldi r22, 10

pentagrama_1:

    ; 1 izquierda + 2 abajo
    ldi r21, 1

    rcall pasos_izquierda
    ldi r21, 2
    rcall pasos_abajo

   ; 1 izquierda + 2 abajo
   ldi r21, 1

    rcall pasos_izquierda
    ldi r21, 2
    rcall pasos_abajo

    ; 1 izquierda + 3 abajo
    ldi r21, 1
    rcall pasos_izquierda
    ldi r21, 3
    rcall pasos_abajo

    ; 1 izquierda + 2 abajo
    ldi r21, 1
    rcall pasos_izquierda
    ldi r21, 2
    rcall pasos_abajo

    dec r22
    brne pentagrama_1

    ldi r22, 10

pentagrama_2:

    ; Repetimos 5 veces:
    ; 2 derecha + 1 arriba

    ldi r21, 2
    rcall pasos_derecha
    ldi r21, 1
    rcall pasos_arriba

    ldi r21, 2
    rcall pasos_derecha
    ldi r21, 1
    rcall pasos_arriba

    ldi r21, 2
    rcall pasos_derecha
    ldi r21, 1
    rcall pasos_arriba

    ldi r21, 2
    rcall pasos_derecha
    ldi r21, 1
    rcall pasos_arriba

    ; Ultimos 2 derecha SIN subir
    ; para conservar 10 derecha + 4 arriba
    ldi r21, 2
    rcall pasos_derecha

    dec r22
    brne pentagrama_2

    ldi r21, 120
    rcall pasos_izquierda

    ldi r22, 10 

pentagrama_3:

   ; 2 derecha + 1 abajo

    ldi r21, 2
    rcall pasos_derecha
    ldi r21, 1
    rcall pasos_abajo

    ldi r21, 2
    rcall pasos_derecha
    ldi r21, 1
    rcall pasos_abajo

    ldi r21, 2
    rcall pasos_derecha
    ldi r21, 1
    rcall pasos_abajo

    ldi r21, 2
    rcall pasos_derecha
    ldi r21, 1
    rcall pasos_abajo

    ; Ultimos 2 derecha
    ldi r21, 2
    rcall pasos_derecha

    dec r22
    brne pentagrama_3

    ldi r22, 10

pentagrama_4:

     ; 1 izquierda + 2 arriba
    ldi r21, 1
    rcall pasos_izquierda
    ldi r21, 2
    rcall pasos_arriba

    ; 1 izquierda + 2 arriba
    ldi r21, 1
    rcall pasos_izquierda
    ldi r21, 2
    rcall pasos_arriba

    ; 1 izquierda + 3 arriba
    ldi r21, 1
    rcall pasos_izquierda
    ldi r21, 3
    rcall pasos_arriba

    ; 1 izquierda + 2 arriba
    ldi r21, 1
    rcall pasos_izquierda
    ldi r21, 2
    rcall pasos_arriba

    dec r22
    brne pentagrama_4

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

    ldi r21, 16

corazon_1:

    rcall diagonal_arriba_izquierda

    dec r21
    brne corazon_1


    ;----------------------------------------------
    ; Parte superior izquierda
    ;----------------------------------------------

    ldi r21, 16

corazon_2:

    rcall diagonal_abajo_izquierda

    dec r21
    brne corazon_2


    ;----------------------------------------------
    ; Lado izquierdo hacia la punta inferior
    ;----------------------------------------------

    ldi r21, 32

corazon_3:

    rcall diagonal_abajo_derecha

    dec r21
    brne corazon_3


    ;----------------------------------------------
    ; Desde la punta inferior hacia lado derecho
    ;----------------------------------------------

    ldi r21, 32

corazon_4:

    rcall diagonal_arriba_derecha

    dec r21
    brne corazon_4


    ;----------------------------------------------
    ; Parte superior derecha
    ;----------------------------------------------

    ldi r21, 16

corazon_5:

    rcall diagonal_arriba_izquierda

    dec r21
    brne corazon_5


    ;----------------------------------------------
    ; Volver al centro superior
    ;----------------------------------------------

    ldi r21, 16

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

    ; Posicion inicial: llevar el dibujo mas hacia el centro.
    ; En este montaje pasos_derecha mueve FISICAMENTE a la izquierda.
    ; 260 pasos no entran en un registro de 8 bits.
    ; Se divide en 130 + 130 = 260.
    ldi r21, 130
    rcall pasos_derecha

    ldi r21, 130
    rcall pasos_derecha

    ldi r21, 180
    rcall pasos_abajo

    ; Dibujar silueta exterior
    ldi ZH, HIGH(GENGAR_MATRIZ<<1)
    ldi ZL, LOW(GENGAR_MATRIZ<<1)
    rcall lapiz_abajo

gengar_leer_matriz:
    lpm r20, Z+
    cpi r20, G_FIN
    breq gengar_fin_silueta
    lpm r21, Z+
    rcall gengar_ejecutar_movimiento
    rjmp gengar_leer_matriz

gengar_fin_silueta:
    rcall lapiz_arriba

;Ojo Izquierdo
  ldi r21, 44
  rcall pasos_derecha ; izquierda fisica
  ldi r21, 64
  rcall pasos_abajo
  rcall gengar_ojo_izquierdo

;Ojo derecho
   ldi r21, 64
   rcall pasos_izquierda ; derecha fisica
   rcall gengar_ojo_derecho

;Boca
   ldi r21, 64
   rcall pasos_derecha      ; izquierda fisica
   ldi r21, 40
   rcall pasos_abajo
   rcall gengar_boca_pixel

    ret

; INTERPRETAR MATRIZ
; r20 = direccion logica del dibujo
; r21 = cantidad de pasos
; El eje X fisico esta invertido en este montaje.

gengar_ejecutar_movimiento:
    cpi r20, G_DER
    brne gengar_check_izq
    rjmp gengar_mov_der
gengar_check_izq:
    cpi r20, G_IZQ
    brne gengar_check_arr
    rjmp gengar_mov_izq
gengar_check_arr:
    cpi r20, G_ARR
    brne gengar_check_aba
    rjmp gengar_mov_arr
gengar_check_aba:
    cpi r20, G_ABA
    brne gengar_check_ad
    rjmp gengar_mov_aba
gengar_check_ad:
    cpi r20, G_AD
    brne gengar_check_ai
    rjmp gengar_mov_ad
gengar_check_ai:
    cpi r20, G_AI
    brne gengar_check_bd
    rjmp gengar_mov_ai
gengar_check_bd:
    cpi r20, G_BD
    brne gengar_check_bi
    rjmp gengar_mov_bd
gengar_check_bi:
    cpi r20, G_BI
    brne gengar_mov_fin
    rjmp gengar_mov_bi

gengar_mov_fin:
    ret

gengar_mov_der:
    rcall pasos_izquierda
    ret
gengar_mov_izq:
    rcall pasos_derecha
    ret
gengar_mov_arr:
    rcall pasos_arriba
    ret
gengar_mov_aba:
    rcall pasos_abajo
    ret

gengar_mov_ad:
gengar_mov_ad_loop:
    rcall diagonal_arriba_izquierda
    dec r21
    brne gengar_mov_ad_loop
    ret

gengar_mov_ai:
gengar_mov_ai_loop:
    rcall diagonal_arriba_derecha
    dec r21
    brne gengar_mov_ai_loop
    ret

gengar_mov_bd:
gengar_mov_bd_loop:
    rcall diagonal_abajo_izquierda
    dec r21
    brne gengar_mov_bd_loop
    ret

gengar_mov_bi:
gengar_mov_bi_loop:
    rcall diagonal_abajo_derecha
    dec r21
    brne gengar_mov_bi_loop
    ret

;Ojo Izquierdo 

gengar_ojo_izquierdo:
    rcall lapiz_abajo

    ldi r21, 12
    rcall pasos_abajo

    ldi r21, 8
gengar_oi_bd:
    rcall diagonal_abajo_izquierda ; abajo-derecha fisica
    dec r21
    brne gengar_oi_bd

    ldi r21, 8
    rcall pasos_izquierda ; derecha fisica

    ldi r21, 4
gengar_oi_ad1:
    rcall diagonal_arriba_izquierda ; arriba-derecha fisica
    dec r21
    brne gengar_oi_ad1

    ldi r21, 16
gengar_oi_ai:
    rcall diagonal_arriba_derecha  ; arriba-izquierda fisica
    dec r21
    brne gengar_oi_ai

    ldi r21, 4
    rcall pasos_derecha ; izquierda fisica

    rcall lapiz_arriba
    ret

;Ojo derecho

gengar_ojo_derecho:
    rcall lapiz_abajo

    ldi r21, 16
gengar_od_bi:
    rcall diagonal_abajo_derecha ; abajo-izquierda fisica
    dec r21
    brne gengar_od_bi

    ldi r21, 4
gengar_od_bd:
    rcall diagonal_abajo_izquierda ; abajo-derecha fisica
    dec r21
    brne gengar_od_bd

    ldi r21, 8
    rcall pasos_izquierda ; derecha fisica

    ldi r21, 8
gengar_od_ad:
    rcall diagonal_arriba_izquierda ; arriba-derecha fisica
    dec r21
    brne gengar_od_ad

    ldi r21, 12
    rcall pasos_arriba

    ldi r21, 4
    rcall pasos_derecha; izquierda fisica

    rcall lapiz_arriba
    ret

;Boca + Dientes

gengar_boca_pixel:
    ; Borde exterior
    rcall lapiz_abajo

    ldi r21, 80
    rcall pasos_izquierda ; derecha fisica

    ldi r21, 20
gengar_boca_bi:
    rcall diagonal_abajo_derecha ; abajo-izquierda fisica
    dec r21
    brne gengar_boca_bi

    ldi r21, 40
    rcall pasos_derecha ; izquierda fisica

    ldi r21, 20
gengar_boca_ai:
    rcall diagonal_arriba_derecha ; arriba-izquierda fisica
    dec r21
    brne gengar_boca_ai

    rcall lapiz_arriba

    ; Diente 1:
    ldi r21, 20
    rcall pasos_izquierda
    rcall lapiz_abajo
    ldi r21, 16
    rcall pasos_abajo
    rcall lapiz_arriba
    ldi r21, 16
    rcall pasos_arriba

    ; Diente 2
    ldi r21, 20
    rcall pasos_izquierda
    rcall lapiz_abajo
    ldi r21, 20
    rcall pasos_abajo
    rcall lapiz_arriba
    ldi r21, 20
    rcall pasos_arriba

    ; Diente 3
    ldi r21, 20
    rcall pasos_izquierda
    rcall lapiz_abajo
    ldi r21, 16
    rcall pasos_abajo
    rcall lapiz_arriba

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

delay_paso_med:

    ldi r20, 255

delay_paso_int:

    dec r20
    brne delay_paso_int

    dec r19
    brne delay_paso_med

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

 .db 13,10,"PLOTTER UTEC",13,10,"1 - Triangulo",13,10,"2 - Circulo",13,10,"3 - Pentagrama",13,10,"4 - Figura libre",13,10,"P - Gengar",13,10,"T - Todas",13,10,"Seleccione: ",0

GENGAR_MATRIZ: 

   ; Contorno exterior extraido pixel por pixel de la referencia.
  ; 1 celda de la grilla = 4 pasos.
    .db G_BI, 12
    .db G_ABA, 4
    .db G_BI, 12
    .db G_IZQ, 4
    .db G_AI, 8
    .db G_IZQ, 4
    .db G_BD, 4
    .db G_ABA, 4
    .db G_BI, 8
    .db G_AI, 4
    .db G_IZQ, 8
    .db G_AI, 4
    .db G_IZQ, 4
    .db G_AI, 4
    .db G_IZQ, 4
    .db G_AI, 4
    .db G_IZQ, 8
    .db G_BD, 4
    .db G_ABA, 4
    .db G_BD, 8
    .db G_ABA, 4
    .db G_BD, 4
    .db G_ABA, 4
    .db G_BD, 8
    .db G_ABA, 8
    .db G_BI, 12
    .db G_IZQ, 4
    .db G_BI, 8
    .db G_ABA, 4
    .db G_BI, 4
    .db G_ABA, 4
    .db G_BI, 4
    .db G_ABA, 28
    .db G_BD, 4
    .db G_DER, 8
    .db G_AD, 4
    .db G_ARR, 4
    .db G_AD, 4
    .db G_ARR, 8
    .db G_AD, 4
    .db G_BD, 4
    .db G_ABA, 8
    .db G_BD, 4
    .db G_ABA, 36
    .db G_BD, 4
    .db G_ABA, 4
    .db G_DER, 12
    .db G_AD, 8
    .db G_ARR, 4
    .db G_AD, 4
    .db G_DER, 8
    .db G_BD, 4
    .db G_DER, 20
    .db G_AD, 4
    .db G_DER, 8
    .db G_BD, 4
    .db G_ABA, 4
    .db G_BD, 8
    .db G_DER, 12
    .db G_ARR, 4
    .db G_AD, 4
    .db G_ARR, 36
    .db G_AD, 4
    .db G_ARR, 8
    .db G_AD, 4
    .db G_BD, 4
    .db G_ABA, 8
    .db G_BD, 4
    .db G_ABA, 4
    .db G_BD, 4
    .db G_DER, 8
    .db G_AD, 4
    .db G_ARR, 28
    .db G_AI, 4
    .db G_ARR, 4
    .db G_AI, 4
    .db G_ARR, 4
    .db G_AI, 8
    .db G_IZQ, 4
    .db G_AI, 12
    .db G_ARR, 8
    .db G_AD, 8
    .db G_ARR, 4
    .db G_AD, 4
    .db G_ARR, 4
    .db G_AD, 8
    .db G_ARR, 4
    .db G_AD, 4
    .db G_IZQ, 8
    .db G_BI, 4
    .db G_IZQ, 4
    .db G_BI, 4
    .db G_IZQ, 4
    .db G_BI, 4
    .db G_IZQ, 8
    .db G_BI, 4
    .db G_AI, 8
    .db G_ARR, 4
    .db G_AD, 4
    .db G_IZQ, 4
    .db G_BI, 8
    .db G_IZQ, 4
    .db G_AI, 4
    .db G_ARR, 8
    .db G_AD, 4
    .db G_ARR, 12
    .db G_IZQ, 4

    .db G_FIN, 0

