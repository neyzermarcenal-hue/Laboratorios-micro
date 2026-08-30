.include "m328pdef.inc"


; Registros que vamos a usar:

.def temp  = R16        ; Registro temporal para hacer configuraciones
.def carga = R17        ; Guarda la carga seleccionada:
                        ; 0 = ligera
                        ; 1 = media
                        ; 2 = pesada


.org 0x0000
    rjmp INICIO         ; Al arrancar el micro, va al inicio del programa



INICIO:


    ; Configuro el Stack Pointer:

    LDI temp, HIGH(RAMEND)    ; Carga la parte alta de la última dirección SRAM
    OUT SPH, temp             ; La guarda en la parte alta del Stack Pointer

    LDI temp, LOW(RAMEND)     ; Carga la parte baja de la última dirección SRAM
    OUT SPL, temp             ; La guarda en la parte baja del Stack Pointer



		/* Configuro las salidas del PORTB
		  PB0 = listo
		  PB1 = lavado
	      PB2 = centrifugado
		  PB3 = secado
		  PB4 = fin
		  PB5 = carga ligera */

	
	LDI temp, 0b00111111      ; Pongo PB0 hasta PB5 como salidas
    OUT DDRB, temp            ; Guardo esa configuración en DDRB



		/* Configuro las salidas del PORTC
		   PC0 = carga media
		   PC1 = carga pesada
		   PC2 = motor derecha
		   PC3 = motor izquierda */

		
	LDI temp, 0b00001111      ; PC0 hasta PC3 van a ser salidas
    OUT DDRC, temp            ; GuardO la configuración en DDRC


		/* Entradas del sistema
		   PD2 = START
		   PD3 = selección de carga
		   PD4 = sensor de puerta
		   PD5 = sensor de nivel */

	
    CLR temp                   ; temp = 0
    OUT DDRD, temp             ; DejO PORTD como entrada


    ; Al comenzar apago todas las salidas

    CLR temp
    OUT PORTB, temp            ; Apaga las salidas del PORTB
    OUT PORTC, temp            ; Apaga las salidas del PORTC


    ; Activo las resistencias pull-up de PD2 a PD5


    LDI temp, 0b00111100      ; Pongo en 1 PD2, PD3, PD4 y PD5
    OUT PORTD, temp            ; Activa las pull-up internas


    ; La lavadora empieza con carga ligera

    CLR carga                  ; carga = 0, entonces empieza en ligera

    SBI PORTB, PB0             ; Enciende LED de listo
    SBI PORTB, PB5             ; Enciende LED de carga ligera


LISTO:

    SBIS PIND, PD3             ; Si PD3 está en 1, el botón no está presionado
    RJMP CAMBIAR_CARGA         ; Si está en 0, se presionó el botón de carga

    RJMP LISTO                 ; Si no pasó nada, sigue esperando


CAMBIAR_CARGA:

    ; Esperamos a que se suelte el botón antes de seguir

ESPERA_CARGA:

    SBIS PIND, PD3             ; Revisa si el botón ya volvió a 1
    RJMP ESPERA_CARGA          ; Si sigue apretado, se queda esperando

    INC carga                  ; Pasa a la siguiente carga / Incrementa 1

	CPI carga, 3               ; Compara con 3
    BRNE ACTUALIZAR_CARGA      ; Si no llegó a 3, seguimos normalmente

	CLR carga                  ; Si llegó a 3, vuelve a 0 = ligera



ACTUALIZAR_CARGA:

    CBI PORTB, PB5             ; Apaga LED de carga ligera
    CBI PORTC, PC0             ; Apaga LED de carga media
    CBI PORTC, PC1             ; Apaga LED de carga pesada

    CPI carga, 0               ; ¿La carga es ligera?
    BREQ MOSTRAR_LIGERA

    CPI carga, 1               ; ¿La carga es media?
    BREQ MOSTRAR_MEDIA

    ; Si no es 0 ni 1, entonces es pesada

MOSTRAR_PESADA:

    SBI PORTC, PC1             ; Enciende LED de carga pesada
    RJMP LISTO


MOSTRAR_MEDIA:

    SBI PORTC, PC0             ; Enciende LED de carga media
    RJMP LISTO


MOSTRAR_LIGERA:

    SBI PORTB, PB5             ; Enciende LED de carga ligera
    RJMP LISTO
