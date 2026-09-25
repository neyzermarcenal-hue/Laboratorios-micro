.INCLUDE "m328pdef.inc"

; Vector de RESET:
.ORG 0x0000
    RJMP INICIO                    ; Al encender, comenzar en INICIO

.ORG 0x0034                        ; Programa principal


; Registros:
.DEF TEMP      = R16               ; Registro auxiliar
.DEF DATO      = R17               ; Dato para DAC o USART
.DEF OPCION    = R18               ; Guarda señal seleccionada
.DEF TEMP2     = R19               ; Segundo registro auxiliar
.DEF CONTADOR  = R20               ; Cuenta las 256 muestras
.DEF VELOCIDAD = R21               ; Nivel de velocidad


; Inicio:
INICIO:

    ; Configurar Stack Pointer
    LDI TEMP, HIGH(RAMEND)         ; Parte alta del final de SRAM
    OUT SPH, TEMP                  ; Configurar SPH

    LDI TEMP, LOW(RAMEND)          ; Parte baja del final de SRAM
    OUT SPL, TEMP                  ; Configurar SPL


    ; Salidas del DAC:
    ; PB0-PB5 = bits 0 a 5
    LDI TEMP, 0b00111111
    OUT DDRB, TEMP                 ; PB0 a PB5 como salidas

    ; PC0-PC1 = bits 6 y 7
    LDI TEMP, 0b00000011
    OUT DDRC, TEMP                 ; PC0 y PC1 como salidas

    ; DAC comienza en cero
    CLR TEMP
    OUT PORTB, TEMP                ; Bits bajos = 0
    OUT PORTC, TEMP                ; Bits altos = 0


    ; USART: 9600 baudios
    LDI TEMP, HIGH(103)            ; Parte alta del divisor
    STS UBRR0H, TEMP

    LDI TEMP, LOW(103)             ; Parte baja del divisor
    STS UBRR0L, TEMP

    ; Habilitar TX y RX
    LDI TEMP, (1<<TXEN0)|(1<<RXEN0)
    STS UCSR0B, TEMP               ; Transmitir y recibir

    ; Formato 8N1
    LDI TEMP, (1<<UCSZ01)|(1<<UCSZ00)
    STS UCSR0C, TEMP               ; 8 bits, sin paridad, 1 stop


; Bucle principal:
PRINCIPAL:

    RJMP PRINCIPAL                 ; Por ahora permanecer aquí