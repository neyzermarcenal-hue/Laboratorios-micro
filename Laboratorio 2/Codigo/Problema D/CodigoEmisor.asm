.include "m328pdef.inc"

.org 0x0000
    rjmp main

main:
    ;Stack Pointer 
    ldi r16, HIGH(RAMEND)
    out SPH, r16

    ldi r16, LOW(RAMEND)
    out SPL, r16

 ;Configuracion de Pines
    in r16, DDRD
    andi r16, ~((1<<2) | (1<<3) | (1<<4)) ; Configurar PD2..PD4 como entradas
    out DDRD, r16

    in r16, PORTD
    andi r16, ~((1<<2) | (1<<3) | (1<<4)) ; Desactivar pull-ups (asume pull-down externas)
    out PORTD, r16

	;Configuracion USART
    clr r16
    sts UBRR0H, r16

    ldi r16, 103
    sts UBRR0L, r16

    ; Habilitar ÚNICAMENTE Transmisor
    ldi r16, (1 << TXEN0)
    sts UCSR0B, r16

    ; 8 bits de datos, 1 bit de parada
    ldi r16, (1 << UCSZ01) | (1 << UCSZ00)
    sts UCSR0C, r16

;Leer y enviar
loop_tx:
    ; Primera prueba: Leer el puerto D completo y enviarlo
    in r16, PIND
	; Ajustar los bits PD2, PD3 y PD4 a la posición 0, 1 y 2
    lsr r16
    lsr r16
    andi r16, 0x07  ; Conservar solo el valor de 3 bits (0 a 7)

wait_tx:
    ; Esperar a que el buffer de transmisión esté listo
    lds r17, UCSR0A
    sbrs r17, UDRE0
    rjmp wait_tx

    ; Transmitir el valor crudo
    sts UDR0, r16

    rjmp loop_tx

