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


	    ; Configurar Timer1 en modo CTC:
    CLR TEMP                       ; TEMP = 0
    STS TCCR1A, TEMP               ; Timer1 sin salidas especiales

    ; Valor de comparación = 249
    ; Con 16 MHz y prescaler 64 genera aprox. 1 ms
    LDI TEMP, HIGH(249)            ; Parte alta de 249
    STS OCR1AH, TEMP               ; Guardar en OCR1A alto

    LDI TEMP, LOW(249)             ; Parte baja de 249
    STS OCR1AL, TEMP               ; Guardar en OCR1A bajo

    ; Activar modo CTC y prescaler de 64
    LDI TEMP, (1<<WGM12)|(1<<CS11)|(1<<CS10)
    STS TCCR1B, TEMP               ; Iniciar Timer1


; Bucle principal:
PRINCIPAL:

    RJMP GENERAR_11                ; Generar Señal 11 continuamente



; Salida al DAC:
DAC_SALIDA:

    MOV TEMP, DATO                 ; Copiar muestra
    ANDI TEMP, 0x3F                ; Conservar bits 0 a 5
    OUT PORTB, TEMP                ; Enviar bits bajos al DAC

    MOV TEMP2, DATO                ; Copiar nuevamente la muestra

    LSR TEMP2                      ; Desplazar bit 6 hacia posición 0
    LSR TEMP2
    LSR TEMP2
    LSR TEMP2
    LSR TEMP2
    LSR TEMP2

    ANDI TEMP2, 0x03               ; Conservar solamente bits 6 y 7
    OUT PORTC, TEMP2               ; Enviar bits altos al DAC

    RET


; Esperar al Timer1:
ESPERAR_TIMER:

    IN TEMP, TIFR1                 ; Leer banderas del Timer1
    SBRS TEMP, OCF1A               ; ¿Ocurrió comparación?
    RJMP ESPERAR_TIMER             ; No ? seguir esperando

    LDI TEMP, (1<<OCF1A)           ; Preparar limpieza de bandera
    OUT TIFR1, TEMP                ; Limpiar OCF1A escribiendo un 1

    RET


; Generar Señal 11:
GENERAR_11:

    LDI ZH, HIGH(LUT_11<<1)        ; Z apunta al inicio de LUT 11
    LDI ZL, LOW(LUT_11<<1)

    CLR CONTADOR                   ; Comenzar en muestra 0


BUCLE_11:

    LPM DATO, Z+                   ; Leer siguiente muestra desde Flash
    RCALL DAC_SALIDA               ; Enviar muestra al DAC
    RCALL ESPERAR_TIMER            ; Esperar hasta siguiente muestra

    INC CONTADOR                   ; Avanzar contador
    BRNE BUCLE_11                  ; 256 muestras hasta volver a cero

    RJMP GENERAR_11                ; Repetir señal continuamente



; LUT Señal 11:
LUT_11:

    .DB 0x00, 0x03, 0x07, 0x0B, 0x0F, 0x13, 0x17, 0x1B, 0x1F, 0x23, 0x27, 0x2B, 0x2F, 0x33, 0x37, 0x3B
    .DB 0x3F, 0x43, 0x47, 0x4B, 0x4F, 0x53, 0x57, 0x5B, 0x5F, 0x63, 0x67, 0x6B, 0x6F, 0x73, 0x77, 0x7B
    .DB 0x7F, 0x83, 0x87, 0x8B, 0x8F, 0x93, 0x97, 0x9B, 0x9F, 0xA3, 0xA7, 0xAB, 0xAF, 0xB3, 0xB7, 0xBB
    .DB 0xBF, 0xC3, 0xC7, 0xCB, 0xCF, 0xD3, 0xD7, 0xDB, 0xDF, 0xE3, 0xE7, 0xEB, 0xEF, 0xF3, 0xF7, 0xFB
    .DB 0xFF, 0xFB, 0xF7, 0xF3, 0xEF, 0xEB, 0xE7, 0xE3, 0xDF, 0xDB, 0xD7, 0xD3, 0xCF, 0xCB, 0xC7, 0xC3
    .DB 0xBF, 0xBB, 0xB7, 0xB3, 0xAF, 0xAB, 0xA7, 0xA3, 0x9F, 0x9B, 0x97, 0x93, 0x8F, 0x8B, 0x87, 0x83
    .DB 0x7F, 0x7B, 0x77, 0x73, 0x6F, 0x6B, 0x67, 0x63, 0x5F, 0x5B, 0x57, 0x53, 0x4F, 0x4B, 0x47, 0x43
    .DB 0x3F, 0x3B, 0x37, 0x33, 0x2F, 0x2B, 0x27, 0x23, 0x1F, 0x1B, 0x17, 0x13, 0x0F, 0x0B, 0x07, 0x03
    .DB 0x00, 0x03, 0x07, 0x0B, 0x0F, 0x13, 0x17, 0x1B, 0x1F, 0x23, 0x27, 0x2B, 0x2F, 0x33, 0x37, 0x3B
    .DB 0x3F, 0x43, 0x47, 0x4B, 0x4F, 0x53, 0x57, 0x5B, 0x5F, 0x63, 0x67, 0x6B, 0x6F, 0x73, 0x77, 0x7B
    .DB 0x7F, 0x83, 0x87, 0x8B, 0x8F, 0x93, 0x97, 0x9B, 0x9F, 0xA3, 0xA7, 0xAB, 0xAF, 0xB3, 0xB7, 0xBB
    .DB 0xBF, 0xC3, 0xC7, 0xCB, 0xCF, 0xD3, 0xD7, 0xDB, 0xDF, 0xE3, 0xE7, 0xEB, 0xEF, 0xF3, 0xF7, 0xFB
    .DB 0xFF, 0xFB, 0xF7, 0xF3, 0xEF, 0xEB, 0xE7, 0xE3, 0xDF, 0xDB, 0xD7, 0xD3, 0xCF, 0xCB, 0xC7, 0xC3
    .DB 0xBF, 0xBB, 0xB7, 0xB3, 0xAF, 0xAB, 0xA7, 0xA3, 0x9F, 0x9B, 0x97, 0x93, 0x8F, 0x8B, 0x87, 0x83
    .DB 0x7F, 0x7B, 0x77, 0x73, 0x6F, 0x6B, 0x67, 0x63, 0x5F, 0x5B, 0x57, 0x53, 0x4F, 0x4B, 0x47, 0x43
    .DB 0x3F, 0x3B, 0x37, 0x33, 0x2F, 0x2B, 0x27, 0x23, 0x1F, 0x1B, 0x17, 0x13, 0x0F, 0x0B, 0x07, 0x03