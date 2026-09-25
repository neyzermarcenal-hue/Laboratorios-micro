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



    ; Selección inicial:
    LDI OPCION, '1'                ; Al iniciar seleccionar Señal 11
    LDI VELOCIDAD, 1               ; Nivel medio de velocidad al iniciar
    RCALL MOSTRAR_MENU             ; Mostrar menú por USART


; Bucle principal:
PRINCIPAL:

    RCALL REVISAR_UART             ; Revisar si llegó una opción por Serial

    CPI OPCION, '1'                ; ¿Está seleccionada Señal 11?
    BRNE COMPROBAR_3               ; No ? comprobar Señal 3

    RCALL GENERAR_11               ; Sí ? generar Señal 11
    RJMP PRINCIPAL                 ; Volver al menú principal


COMPROBAR_3:

    CPI OPCION, '2'                ; ¿Está seleccionada Señal 3?
    BRNE PRINCIPAL                 ; No ? seguir esperando

    RCALL GENERAR_3                ; Sí ? generar Señal 3
    RJMP PRINCIPAL                 ; Volver



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

    LDI ZH, HIGH(LUT_11<<1)        ; Apuntar al inicio de LUT 11
    LDI ZL, LOW(LUT_11<<1)

    CLR CONTADOR                   ; Comenzar desde muestra 0


BUCLE_11:

    LPM DATO, Z+                   ; Leer muestra de LUT

    RCALL DAC_SALIDA               ; Enviar muestra al DAC

    RCALL ESPERAR_TIMER            ; Esperar tiempo entre muestras

    RCALL REVISAR_UART             ; Ver si usuario cambió de señal

    CPI OPCION, '1'                ; ¿Sigue seleccionada Señal 11?
    BRNE FIN_11                    ; No ? salir

    INC CONTADOR                   ; Siguiente muestra
    BRNE BUCLE_11                  ; Repetir las 256 muestras

    RJMP GENERAR_11                ; Reiniciar señal periódica


FIN_11:

    RET                            ; Volver a PRINCIPAL



; Generar Señal 3:
GENERAR_3:

    LDI ZH, HIGH(LUT_3<<1)         ; Apuntar al inicio de LUT 3
    LDI ZL, LOW(LUT_3<<1)

    CLR CONTADOR                   ; Comenzar desde muestra 0


BUCLE_3:

    LPM DATO, Z+                   ; Leer muestra de LUT

    RCALL DAC_SALIDA               ; Enviar muestra al DAC

    RCALL ESPERAR_TIMER            ; Esperar tiempo entre muestras

    RCALL REVISAR_UART             ; Revisar puerto Serial

    CPI OPCION, '2'                ; ¿Sigue seleccionada Señal 3?
    BRNE FIN_3                     ; No ? salir

    INC CONTADOR                   ; Siguiente muestra
    BRNE BUCLE_3                   ; Repetir las 256 muestras

    RJMP GENERAR_3                 ; Volver al inicio de la rampa


FIN_3:

    RET                            ; Volver al programa principal


; USART: enviar un carácter
USART_TX:

    LDS TEMP, UCSR0A               ; Leer estado del USART

    SBRS TEMP, UDRE0               ; ¿Registro de transmisión disponible?
    RJMP USART_TX                  ; No ? seguir esperando

    STS UDR0, DATO                 ; Sí ? enviar carácter

    RET                            ; Volver


; USART: enviar un texto
USART_TEXTO:

    LPM DATO, Z+                   ; Leer siguiente carácter desde Flash

    TST DATO                       ; ¿El carácter es cero?
    BREQ USART_TEXTO_FIN           ; Sí ? terminó el mensaje

    RCALL USART_TX                 ; Enviar carácter

    RJMP USART_TEXTO               ; Leer el siguiente


USART_TEXTO_FIN:

    RET                            ; Volver


; Mostrar menú:
MOSTRAR_MENU:

    PUSH ZL                        ; Guardar parte baja del puntero Z
    PUSH ZH                        ; Guardar parte alta del puntero Z

    LDI ZH, HIGH(TEXTO_MENU<<1)    ; Z apunta al texto del menú
    LDI ZL, LOW(TEXTO_MENU<<1)

    RCALL USART_TEXTO              ; Enviar menú completo

    POP ZH                         ; Recuperar Z
    POP ZL

    RET                            ; Volver



; Revisar recepción USART:
REVISAR_UART:

    LDS TEMP, UCSR0A               ; Leer estado del USART

    SBRS TEMP, RXC0                ; ¿Llegó un carácter?
    RET                            ; No ? volver inmediatamente

    LDS DATO, UDR0                 ; Leer carácter recibido


    CPI DATO, '1'                  ; ¿Usuario escribió 1?
    BREQ SELECCION_11              ; Sí ? seleccionar Señal 11

    CPI DATO, '2'                  ; ¿Usuario escribió 2?
    BREQ SELECCION_3               ; Sí ? seleccionar Señal 3

    CPI DATO, '+'                  ; ¿Usuario quiere más velocidad?
    BREQ MAS_RAPIDO

    CPI DATO, '-'                  ; ¿Usuario quiere menos velocidad?
    BREQ MAS_LENTO

    CPI DATO, 'M'                  ; ¿Usuario escribió M?
    BREQ MOSTRAR_MENU_UART

    CPI DATO, 'm'                  ; También aceptar m minúscula
    BREQ MOSTRAR_MENU_UART

    RET                            ; Otro carácter ? ignorarlo


SELECCION_11:

    LDI OPCION, '1'                ; Guardar selección de Señal 11
    RET


SELECCION_3:

    LDI OPCION, '2'                ; Guardar selección de Señal 3
    RET

; Aumentar velocidad:
MAS_RAPIDO:

    CPI VELOCIDAD, 2               ; ¿Ya estamos en nivel máximo?
    BREQ FIN_MAS_RAPIDO            ; Sí ? no aumentar más

    INC VELOCIDAD                  ; Subir un nivel

    RCALL CONFIG_VELOCIDAD         ; Actualizar Timer1


FIN_MAS_RAPIDO:

    RET


; Disminuir velocidad:
MAS_LENTO:

    CPI VELOCIDAD, 0               ; ¿Ya estamos en nivel mínimo?
    BREQ FIN_MAS_LENTO             ; Sí ? no disminuir más

    DEC VELOCIDAD                  ; Bajar un nivel

    RCALL CONFIG_VELOCIDAD         ; Actualizar Timer1


FIN_MAS_LENTO:

    RET


MOSTRAR_MENU_UART:

    RCALL MOSTRAR_MENU             ; Mostrar nuevamente opciones
    RET


; Configurar velocidad del Timer1:
CONFIG_VELOCIDAD:

    CPI VELOCIDAD, 0               ; ¿Nivel lento?
    BREQ VELOCIDAD_LENTA

    CPI VELOCIDAD, 1               ; ¿Nivel medio?
    BREQ VELOCIDAD_MEDIA

    RJMP VELOCIDAD_RAPIDA          ; Si no, nivel rápido


; Velocidad lenta:
VELOCIDAD_LENTA:

    LDI TEMP, HIGH(499)            ; 499 ? aproximadamente 2 ms
    STS OCR1AH, TEMP

    LDI TEMP, LOW(499)
    STS OCR1AL, TEMP

    RJMP REINICIAR_TIMER


; Velocidad media:
VELOCIDAD_MEDIA:

    LDI TEMP, HIGH(249)            ; 249 ? aproximadamente 1 ms
    STS OCR1AH, TEMP

    LDI TEMP, LOW(249)
    STS OCR1AL, TEMP

    RJMP REINICIAR_TIMER


; Velocidad rápida:
VELOCIDAD_RAPIDA:

    LDI TEMP, HIGH(124)            ; 124 ? aproximadamente 0.5 ms
    STS OCR1AH, TEMP

    LDI TEMP, LOW(124)
    STS OCR1AL, TEMP


; Reiniciar Timer después del cambio:
REINICIAR_TIMER:

    CLR TEMP                       ; TEMP = 0

    STS TCNT1H, TEMP               ; Reiniciar parte alta
    STS TCNT1L, TEMP               ; Reiniciar parte baja

    LDI TEMP, (1<<OCF1A)           ; Limpiar bandera de comparación
    OUT TIFR1, TEMP

    RET


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


; LUT Señal 3:
LUT_3:

    .DB 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F
    .DB 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F
    .DB 0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F
    .DB 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F
    .DB 0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F
    .DB 0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F
    .DB 0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F
    .DB 0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,0x7F
    .DB 0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F
    .DB 0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F
    .DB 0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF
    .DB 0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF
    .DB 0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF
    .DB 0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF
    .DB 0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF
    .DB 0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF



; Menú USART:
TEXTO_MENU:

    .DB 13,10,"DAC R-2R",13,10,"1 - Senal 11",13,10,"2 - Senal 3",13,10,"+ - Mas rapido",13,10,"- - Mas lento",13,10,"M - Menu",13,10,0,0