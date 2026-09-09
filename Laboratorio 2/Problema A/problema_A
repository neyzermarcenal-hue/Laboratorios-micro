.include "m328pdef.inc"

.def temp    = r16
.def dato    = r17 ; Guarda el patrón de LEDs que corresponde a una fila.
.def indice  = r18 ; Indica qué fila estamos mostrando.
.def figura  = r19 ; Guarda cuál dibujo está seleccionado
.def mascara = r20 ; Se utiliza para seleccionar una sola fila de la matriz.

; Registros para el mensaje
.def posicion = r21
.def columna = r22
.def contador = r23

.equ LIMITE_MENSAJE = 101

.org 0x0000
    rjmp reset


reset:

    ; =========================
    ; Configuro el Stack
    ; =========================

    ldi temp, HIGH(RAMEND)
    out SPH, temp

    ldi temp, LOW(RAMEND)
    out SPL, temp

    ; =========================
    ; Configuro los puertos
    ; =========================

    ; PD2 a PD7 son 6 filas
    ; PD0 y PD1 quedan para USART
    ldi temp, 0b11111100
    out DDRD, temp

    ; Empiezo con las filas apagadas
    ldi temp, 0b11111100
    out PORTD, temp


    ; PC0 a PC5 son 6 columnas
    ldi temp, 0x3F
    out DDRC, temp

    ; Columnas apagadas
    ldi temp, 0x00
    out PORTC, temp


    ; PB0 y PB1 son las otras 2 filas
    ; PB2 y PB3 son las otras 2 columnas
    ldi temp, 0b00001111
    out DDRB, temp

    ; PB0 y PB1 en 1 = filas apagadas
    ; PB2 y PB3 en 0 = columnas apagadas
    ldi temp, 0b00000011
    out PORTB, temp

    ; =========================
    ; Inicializo UART
    ; =========================

    rcall INIT_UART


    ; 255 significa que todavía no seleccioné nada
    ldi figura, 255

    ; Empiezo desde la primera columna del mensaje
    clr posicion


    ; Muestro el mensaje inicial en la terminal
    ldi ZH, HIGH(TEXTO_MENU*2)
    ldi ZL, LOW(TEXTO_MENU*2)
    rcall ENVIAR_CADENA

main:

    ; Reviso si llegó algo por UART
    rcall REVISAR_UART

    ; 0 = UWU
    cpi figura, 0
    breq MOSTRAR_UWU

    ; 1 = XD
    cpi figura, 1
    breq MOSTRAR_XD

    ; 2 = Mensaje
    cpi figura, 2
    breq MOSTRAR_MENSAJE

    ; Si todavía no elegí nada
    rjmp main

; =========================
; Revisar UART
; =========================

REVISAR_UART:

    ; Miro si llegó un carácter
    lds temp, UCSR0A
    sbrs temp, RXC0
    ret

    ; Leo el carácter recibido
    lds temp, UDR0


    ; ASCII 1
    cpi temp, 0x31
    breq SELECCION_MENSAJE

    ; ASCII 2
    cpi temp, 0x32
    breq SELECCION_UWU

    ; ASCII 3
    cpi temp, 0x33
    breq SELECCION_XD

    ; Si no es 1, 2 o 3 no hago nada
    ret


SELECCION_MENSAJE:

    ldi figura, 2

    ; El mensaje vuelve a empezar
    clr posicion

    ret


SELECCION_UWU:

    ldi figura, 0
    ret


SELECCION_XD:

    ldi figura, 1
    ret


; =========================
; Mostrar UWU
; =========================

MOSTRAR_UWU:

    ldi indice, 0


BUCLE_UWU:

    rcall MOSTRAR_FILA_UWU

    inc indice

    cpi indice, 8
    brlo BUCLE_UWU

    rjmp main


; =========================
; Mostrar XD
; =========================

MOSTRAR_XD:

    ldi indice, 0


BUCLE_XD:

    rcall MOSTRAR_FILA_XD

    inc indice

    cpi indice, 8
    brlo BUCLE_XD

    rjmp main

; =========================
; Mostrar mensaje 
; =========================

MOSTRAR_MENSAJE:
    ldi contador, 35 ; repito varias veces la misma posicion para que el desplazamiento no sea demasiado rápido

REFRESCAR_MENSAJE:
    ldi indice, 0

BUCLE_MENSAJE:
    rcall MOSTRAR_FILA_MENSAJE

    inc indice

    cpi indice, 8 
    brlo BUCLE_MENSAJE

    dec contador
    brne REFRESCAR_MENSAJE

    ; Cuando termine de refrescar la imagen, avanzo una columna en el mensaje
    inc posicion

    cpi posicion, LIMITE_MENSAJE
    brlo CONTINUAR_MENSAJE

    clr posicion

CONTINUAR_MENSAJE:
    rjmp main


; =========================
; Filas UWU
; =========================

MOSTRAR_FILA_UWU:

    ; Apago toda la matriz
    rcall APAGAR_MATRIZ

    ; Selecciono la fila actual
    rcall SELECCIONAR_FILA


    ; Busco el patrón UWU
    ldi ZH, HIGH(UWU*2)
    ldi ZL, LOW(UWU*2)

    add ZL, indice
    clr temp
    adc ZH, temp

    lpm dato, Z


    rcall ESCRIBIR_COLUMNAS
    rcall DELAY

    ret

; =========================
; Filas XD
; =========================

MOSTRAR_FILA_XD:

    ; Apago toda la matriz
    rcall APAGAR_MATRIZ

    ; Selecciono la fila actual
    rcall SELECCIONAR_FILA


    ; Busco el patrón XD
    ldi ZH, HIGH(XD*2)
    ldi ZL, LOW(XD*2)

    add ZL, indice
    clr temp
    adc ZH, temp

    lpm dato, Z


    rcall ESCRIBIR_COLUMNAS
    rcall DELAY

    ret

; =========================
; Filas del mensaje
; =========================

MOSTRAR_FILA_MENSAJE:

     ; Apago toda la matriz
    rcall APAGAR_MATRIZ

    ; Selecciono la fila actual
    rcall SELECCIONAR_FILA


    ; Formo los 8 bits que se ven en pantalla
    clr dato

    ldi columna, 0


ARMAR_FILA:

    ; Z apunta al mensaje
    ldi ZH, HIGH(MENSAJE*2)
    ldi ZL, LOW(MENSAJE*2)


    ; Sumo la posición actual
    add ZL, posicion
    clr temp
    adc ZH, temp


    ; Sumo la columna que estoy mostrando
    add ZL, columna
    clr temp
    adc ZH, temp


    ; Leo una columna del mensaje
    lpm mascara, Z


    ; Busco el bit que corresponde
    ; a la fila que estoy mostrando
    mov temp, indice


BUSCAR_BIT_FILA:

    tst temp
    breq BIT_FILA_LISTO

    lsr mascara
    dec temp

    rjmp BUSCAR_BIT_FILA


BIT_FILA_LISTO:

    ; Si el bit 0 está apagado no hago nada
    sbrs mascara, 0
    rjmp SIGUIENTE_COLUMNA


    ; Si está prendido pongo un 1
    ; en la columna correspondiente
    ldi mascara, 1
    mov temp, columna


DESPLAZA_COLUMNA:

    tst temp
    breq COLUMNA_LISTA

    lsl mascara
    dec temp

    rjmp DESPLAZA_COLUMNA


COLUMNA_LISTA:

    or dato, mascara


SIGUIENTE_COLUMNA:

    inc columna

    cpi columna, 8
    brlo ARMAR_FILA


    rcall ESCRIBIR_COLUMNAS
    rcall DELAY

    ret

; =========================
; Apagar matriz
; =========================

APAGAR_MATRIZ:

    ; Apago filas PD2 a PD7
    in temp, PORTD
    ori temp, 0b11111100
    out PORTD, temp


    ; Apago columnas PC0 a PC5
    ldi temp, 0x00
    out PORTC, temp


    ; PB0 y PB1 son filas:
    ; las pongo en 1 para apagarlas

    ; PB2 y PB3 son columnas:
    ; las pongo en 0 para apagarlas
    in temp, PORTB
    andi temp, 0b11110000
    ori temp, 0b00000011
    out PORTB, temp

    ret

; =========================
; Seleccionar fila
; =========================

SELECCIONAR_FILA:

    ; Filas 0 a 5 están en PD2 a PD7
    cpi indice, 6
    brlo FILA_PORTD


    ; Fila 6 está en PB0
    cpi indice, 6
    breq FILA_PB0


    ; Fila 7 está en PB1
    cbi PORTB, PB1
    ret


FILA_PB0:

    cbi PORTB, PB0
    ret


FILA_PORTD:

    ; PD2 corresponde a la primera fila
    ldi mascara, 0b00000100

    mov temp, indice


DESPLAZA_FILA_PORTD:

    tst temp
    breq FILA_PORTD_LISTA

    lsl mascara
    dec temp

    rjmp DESPLAZA_FILA_PORTD


FILA_PORTD_LISTA:

    ; Las filas funcionan con lógica activa en 0
    com mascara

    in temp, PORTD
    and temp, mascara
    out PORTD, temp

    ret


; =========================
; Escritura de columnas
; =========================

ESCRIBIR_COLUMNAS:

    ; Primeras 6 columnas por PORTC
    mov temp, dato
    andi temp, 0b00111111
    out PORTC, temp


    ; Columna 7 por PB2
    sbrc dato, 6
    sbi PORTB, PB2

    sbrs dato, 6
    cbi PORTB, PB2


    ; Columna 8 por PB3
    sbrc dato, 7
    sbi PORTB, PB3

    sbrs dato, 7
    cbi PORTB, PB3

    ret


; =========================
; Inicializar UART
; =========================

INIT_UART:

    ; UBRR = 103 para 9600 baudios
    ; trabajando a 16 MHz

    ldi temp, 0
    sts UBRR0H, temp

    ldi temp, 103
    sts UBRR0L, temp


    ; Habilito recepción y transmisión
    ldi temp, (1<<RXEN0) | (1<<TXEN0)
    sts UCSR0B, temp


    ; 8 bits de datos
    ; 1 bit de stop
    ; sin paridad
    ldi temp, (1<<UCSZ01) | (1<<UCSZ00)
    sts UCSR0C, temp

    ret


; =========================
; Enviar carácter por UART
; =========================

UART_TX:

    ; Guardo el carácter mientras espero
    push temp


ESPERA_TX:

    lds temp, UCSR0A

    sbrs temp, UDRE0
    rjmp ESPERA_TX


    ; Recupero el carácter
    pop temp

    sts UDR0, temp

    ret


; =========================
; Enviar cadena por UART
; =========================

ENVIAR_CADENA:

SIGUIENTE_CARACTER:

    lpm temp, Z+

    tst temp
    breq FIN_CADENA

    rcall UART_TX

    rjmp SIGUIENTE_CARACTER


FIN_CADENA:

    ret


; =========================
; Dibujo UWU
; =========================

UWU:

    .db 0b00000000, 0b00110110
    .db 0b00110110, 0b00000000
    .db 0b01001001, 0b01001001
    .db 0b00110110, 0b00000000


; =========================
; Dibujo XD
; =========================

XD:

    .db 0b10101110, 0b10101001
    .db 0b01001001, 0b01001001
    .db 0b01001001, 0b10101001
    .db 0b10101110, 0b00000000


; =========================
; Mensaje VAMOS MARCELOOOO
;
; cada byte represena UNA COLUMNA de una letra
; se deja una columna en 0 entre las letras
;
; =========================

MENSAJE:

    ; Espacio inicial para que entre desde la derecha
    .db 0x00, 0x00, 0x00, 0x00
    .db 0x00, 0x00, 0x00, 0x00

    ; V
    .db 0b00011111, 0b00100000
    .db 0b01000000, 0b00100000
    .db 0b00011111, 0b00000000

    ; A
    .db 0b01111110, 0b00001001
    .db 0b00001001, 0b00001001
    .db 0b01111110, 0b00000000

    ; M
    .db 0b01111111, 0b00000010
    .db 0b00000100, 0b00000010
    .db 0b01111111, 0b00000000

    ; O
    .db 0b00111110, 0b01000001
    .db 0b01000001, 0b01000001
    .db 0b00111110, 0b00000000

    ; S
    .db 0b01000110, 0b01001001
    .db 0b01001001, 0b01001001
    .db 0b00110001, 0b00000000

    ; Espacio
    .db 0x00, 0x00

    ; M
    .db 0b01111111, 0b00000010
    .db 0b00000100, 0b00000010
    .db 0b01111111, 0b00000000

    ; A
    .db 0b01111110, 0b00001001
    .db 0b00001001, 0b00001001
    .db 0b01111110, 0b00000000

    ; R
    .db 0b01111111, 0b00001001
    .db 0b00011001, 0b00101001
    .db 0b01000110, 0b00000000

    ; C
    .db 0b00111110, 0b01000001
    .db 0b01000001, 0b01000001
    .db 0b00100010, 0b00000000

    ; E
    .db 0b01111111, 0b01001001
    .db 0b01001001, 0b01001001
    .db 0b01000001, 0b00000000

    ; L
    .db 0b01111111, 0b01000000
    .db 0b01000000, 0b01000000
    .db 0b01000000, 0b00000000

    ; O
    .db 0b00111110, 0b01000001
    .db 0b01000001, 0b01000001
    .db 0b00111110, 0b00000000

    ; O
    .db 0b00111110, 0b01000001
    .db 0b01000001, 0b01000001
    .db 0b00111110, 0b00000000

    ; O
    .db 0b00111110, 0b01000001
    .db 0b01000001, 0b01000001
    .db 0b00111110, 0b00000000

    ; O
    .db 0b00111110, 0b01000001
    .db 0b01000001, 0b01000001
    .db 0b00111110, 0b00000000

    ; Espacio final
    .db 0x00, 0x00, 0x00, 0x00
    .db 0x00, 0x00, 0x00, 0x00

; =========================
; Texto del menú UART
; =========================

TEXTO_MENU:

    .db 13, 10
    .db "MATRIZ LED", 13, 10
    .db 13, 10
    .db "1 - VAMOS MARCELOOOO", 13, 10
    .db "2 - UWU", 13, 10, 13
    .db 10, "3 - XD", 13, 10, 0


; =========================
; Delay
; =========================

DELAY:

    ldi r24, 30


D1:

    ldi r25, 50


D2:

    dec r25
    brne D2

    dec r24
    brne D1

    ret
