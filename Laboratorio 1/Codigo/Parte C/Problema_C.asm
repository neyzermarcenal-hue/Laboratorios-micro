.include "m328pdef .inc"

.cseg
.org 0x0000
  rjmp INICIO

INICIO:
  ldi r16, 0xFF
  out DDRD, r16

  ldi r17, 0b00000001

SECUENCIA1:
  out PORTD, r17
  rcall RETARDO

  lsl r17
  brne SECUENCIA1

  ldi r17, 0b00000001
  rjmp SECUENCIA1

SECUENCIA2:
  ldi r17, 0b10000000

BUCLE2:
  out PORTD, r17
  rcall RETARDO

  lsr r17
  brne BUCLE2

  rjmp SECUENCIA 2

SECUENCIA3:
  ldi r17, 0b10101010
  out PORTD, r17
  rcall RETARDO

  ldi r17, 0b01010101
  out PORTD, r17
  rcall RETARDO

  rjmp SECUENCIA3

SECUENCIA4:
  ldi r17, 0b10000001
  out PORTD, r17
  rcall RETARDO

  ldi r17, 0b01000010
  out PORTD, r17
  rcall RETARDO

  ldi r17, 0b00100100
  out PORTD, r17
  rcall RETARDO

  ldi r17, 0b00011000
  out PORTD, r17
  rcall RETARDO

  rjmp SECUENCIA4

SECUENCIA5:
  ldi r17, 0b00011000
  out PORTD, r17
  rcall RETARDO

  ldi r17, 0B00100100
  out PORTD, r17
  rcall RETARDO

  ldi r17, 0b01000010
  out PORTD, r17
  rcall RETARDO

  ldi r17, 0b10000001
  out PORTD, r17
  rcall RETARDO

  rjmp SECUENCIA5

SECUENCIA6:
  ldi r17, 0b00000001
  out PORTD, r17
  rcall RETARDO

  ldi, r17, 0b00000011
  out PORTD, r17
  rcall RETARDO

  ldi, r17, 0b00000111
  out PORTD, r17
  rcall RETARDO

  ldi, r17, 0b00001111
  out PORTD, r17
  rcall RETARDO

  ldi, r17, 0b00011111
  out PORTD, r17
  rcall RETARDO

  ldi, r17, 0b00111111
  out PORTD, r17
  rcall RETARDO

  ldi, r17, 0b01111111
  out PORTD, r17
  rcall RETARDO

  ldi, r17, 0b11111111
  out PORTD, r17
  rcall RETARDO

  rjmp SECUENCIA6

SECUENCIA7:
  ldi, r17, 0b11111111
  out PORTD, r17
  rcall RETARDO

  ldi, r17, 0b01111111
  out PORTD, r17
  rcall RETARDO

  ldi, r17, 0b00111111
  out PORTD, r17
  rcall RETARDO

  ldi, r17, 0b00011111
  out PORTD, r17
  rcall RETARDO

  ldi, r17, 0b00001111
  out PORTD, r17
  rcall RETARDO

  ldi, r17, 0b00000111
  out PORTD, r17
  rcall RETARDO

  ldi, r17, 0b00000011
  out PORTD, r17
  rcall RETARDO

  ldi r17, 0b00000001
  out PORTD, r17
  rcall RETARDO

  rjmp SECUENCIA7

SECUENCIA8:
  ldi, r17, 0b11111111
  out PORTD, r17
  rcall RETARDO

  ldi, r17, 0b00000000
  out PORTD, r17
  rcall RETARDO

  rjmp RETARDO


; =========================================================
; LECTURA DE BOTONES
; =========================================================

LEER_BOTONES:

    sbic PINB, PB0
    rjmp REVISAR_BOTON2

    inc r21

    ; Si pasó de 8, volver a 1
    cpi r21, 9
    brne ESPERAR_B1

    ldi r21, 1

ESPERAR_B1:
    sbis PINB, PB0
    rjmp ESPERAR_B1

    ret

REVISAR_BOTON2:

    sbic PINB, PB1
    rjmp REVISAR_BOTON3

    dec r21

    ; Si bajó de 1, volver a 8
    cpi r21, 0
    brne ESPERAR_B2

    ldi r21, 8

ESPERAR_B2:
    sbis PINB, PB1
    rjmp ESPERAR_B2

    ret

REVISAR_BOTON3:

    sbic PINB, PB2
    rjmp FIN_BOTONES

    ldi r21, 1

ESPERAR_B3:
    sbis PINB, PB2
    rjmp ESPERAR_B3


FIN_BOTONES:
    ret

; =========================================================
; RETARDO
; =========================================================

RETARDO:
  ldi r18, 20

RET1:
   ldi r19, 255

RET2:
  ldi r20, 255

RET3:
  dec r20
  brne RET3

  dec r19
  brne RET2

  dec r18
  brne RET1

  ret


