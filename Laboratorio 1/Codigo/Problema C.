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


