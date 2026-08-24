.include "m328pdef .inc"

.cseg
.org 0x0000
  rjmp INICIO

INICIO:
  ldi r16, 0xFF
  out DDRD, r16

  ; PB0, PB1 y PB2 como entradas
  cbi DDRB, PB0
  cbi DDRB, PB1
  bi DDRB, PB2
  
  ; Pull-up interno de los tres botones 
  sbi PORTB, PB0
  sbi PORTB, PB1
  sbi PORTB, PB2

  ldi r21, 1

  rjmp SELECTOR

SELECTOR:

    cpi r21, 1
    brne SELECTOR2
    rjmp SECUENCIA1

SELECTOR2:
    cpi r21, 2
    brne SELECTOR3
    rjmp SECUENCIA2

SELECTOR3:
    cpi r21, 3
    brne SELECTOR4
    rjmp SECUENCIA3

SELECTOR4:
    cpi r21, 4
    brne SELECTOR5
    rjmp SECUENCIA4

SELECTOR5:
    cpi r21, 5
    brne SELECTOR6
    rjmp SECUENCIA5

SELECTOR6:
    cpi r21, 6
    brne SELECTOR7
    rjmp SECUENCIA6

SELECTOR7:
    cpi r21, 7
    brne SELECTOR8
    rjmp SECUENCIA7

SELECTOR8:
    cpi r21, 8
    brne SELECTOR
    rjmp SECUENCIA8


SECUENCIA1:
  out PORTD, r17
  rcall RETARDO

  lsl LEER_BOTONES
  cpi r21, 1
  brne IR_SELECTOR_S1

  ldi r17

  ;si llegó a cero, reiniciar patrón
  brne BUCLE_S1

  rjmp SECUENCIA1

IR_SELECTOR_S1:
  rjmp SELECTOR

SECUENCIA2:
  ldi r17, 0b10000000

BUCLE_S2:
  out PORTD, r17
  rcall RETARDO

  rcall LEER_BOTONES
  cpi r21, 2
  brne IR_SELECTOR_S2

  lsr r17
  brne BUCLE_S2

  rjmp SECUENCIA 2

IR_SELECTOR_S2:
  rjmp SELECTOR

SECUENCIA3:
  ldi r17, 0b10101010
  out PORTD, r17
  rcall RETARDO

  rcall LEER_BOTONES
  cpi r21, 3
  brne IR_SELECTOR_S3

  ldi r17, 0b01010101
  out PORTD, r17
  rcall RETARDO

  rcall LEER_BOTONES
  cpi r21, 3
  breq SECUENCIA3

IR_SELECTOR_S3:
  rjmp SELECTOR

SECUENCIA4:
  ldi r17, 0b10000001
  out PORTD, r17
  rcall RETARDO

  rcall LEER_BOTONES
  cpi r21, 4
  brne IR_SELECTORS4

  ldi r17, 0b01000010
  out PORTD, r17
  rcall RETARDO

  rcall LEER_BOTONES
  cpi r21, 4
  brne IR_SELECTORS4

  ldi r17, 0b00100100
  out PORTD, r17
  rcall RETARDO

  rcall LEER_BOTONES
  cpi r21, 4
  brne IR_SELECTORS4

  ldi r17, 0b00011000
  out PORTD, r17
  rcall RETARDO

  rcall LEER_BOTONES
  cpi r21, 4
  breq SECUENCIA4

IR_SELECTOR_S4:

  rjmp SELECTOR

SECUENCIA5:
  ldi r17, 0b00011000
  out PORTD, r17
  rcall RETARDO

  rcall LEER_BOTONES
  cpi r21, 5
  brne IR_SELECTOR_S5

  ldi r17, 0B00100100
  out PORTD, r17
  rcall RETARDO

  rcall LEER_BOTONES
  cpi r21, 5
  brne IR_SELECTOR_S5

  ldi r17, 0b01000010
  out PORTD, r17
  rcall RETARDO

  rcall LEER_BOTONES
  cpi r21, 5
  brne IR_SELECTOR_S5

  ldi r17, 0b10000001
  out PORTD, r17
  rcall RETARDO

  rcall LEER_BOTONES
  cpi r21, 5
  breq SECUENCIA5

IR_SELECTOR_S5:
  rjmp SELECTOR

SECUENCIA6:
  ldi r17, 0b00000001
  out PORTD, r17
  rcall RETARDO

  rcall LEER_BOTONES
  cpi r21, 6
  brne IR_SELECTOR_S6

  ldi, r17, 0b00000011
  out PORTD, r17
  rcall RETARDO

  rcall LEER_BOTONES
  cpi r21, 6
  brne IR_SELECTOR_S6

  ldi, r17, 0b00000111
  out PORTD, r17
  rcall RETARDO

  rcall LEER_BOTONES
  cpi r21, 6
  brne IR_SELECTOR_S6

  ldi, r17, 0b00001111
  out PORTD, r17
  rcall RETARDO

  rcall LEER_BOTONES
  cpi r21, 6
  brne IR_SELECTOR_S6

  ldi, r17, 0b00011111
  out PORTD, r17
  rcall RETARDO

  rcall LEER_BOTONES
  cpi r21, 6
  brne IR_SELECTOR_S6

  ldi, r17, 0b00111111
  out PORTD, r17
  rcall RETARDO

  rcall LEER_BOTONES
  cpi r21, 6
  brne IR_SELECTOR_S6

  ldi, r17, 0b01111111
  out PORTD, r17
  rcall RETARDO

  rcall LEER_BOTONES
  cpi r21, 6
  brne IR_SELECTOR_S6

  ldi, r17, 0b11111111
  out PORTD, r17
  rcall RETARDO

  rcall LEER_BOTONES
  cpi r21, 6
  breq SECUENCIA6

IR_SELECTOR_S6:
  rjmp SELECTOR

SECUENCIA7:
  ldi, r17, 0b11111111
  out PORTD, r17
  rcall RETARDO

  rcall LEER_BOTONES
  cpi r21, 7
  brne IR_SELECTOR_S7

  ldi, r17, 0b01111111
  out PORTD, r17
  rcall RETARDO

  rcall LEER_BOTONES
  cpi r21, 7
  brne IR_SELECTOR_S7

  ldi, r17, 0b00111111
  out PORTD, r17
  rcall RETARDO

  rcall LEER_BOTONES
  cpi r21, 7
  brne IR_SELECTOR_S7

  ldi, r17, 0b00011111
  out PORTD, r17
  rcall RETARDO

  rcall LEER_BOTONES
  cpi r21, 7
  brne IR_SELECTOR_S7

  ldi, r17, 0b00001111
  out PORTD, r17
  rcall RETARDO

  rcall LEER_BOTONES
  cpi r21, 7
  brne IR_SELECTOR_S7

  ldi, r17, 0b00000111
  out PORTD, r17
  rcall RETARDO

  rcall LEER_BOTONES
  cpi r21, 7
  brne IR_SELECTOR_S7

  ldi, r17, 0b00000011
  out PORTD, r17
  rcall RETARDO

  rcall LEER_BOTONES
  cpi r21, 7
  brne IR_SELECTOR_S7

  ldi r17, 0b00000001
  out PORTD, r17
  rcall RETARDO

  rcall LEER_BOTONES
  cpi r21, 7
  breq SECUENCIA7

IR_SELECTOR_S7:
  rjmp SELECTOR

SECUENCIA8:
  ldi, r17, 0b11111111
  out PORTD, r17
  rcall RETARDO

  rcall LEER_BOTONES
  cpi r21, 8
  brne IR_SELECTOR_S8

  ldi, r17, 0b00000000
  out PORTD, r17
  rcall RETARDO

  rcall LEER_BOTONES
  cpi r21, 8
  breq SECUENCIA8

IR_SELECTOR_S8
  rjmp SELECTOR

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


