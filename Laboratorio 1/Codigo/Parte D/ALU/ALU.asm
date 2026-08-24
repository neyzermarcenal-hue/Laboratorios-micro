.include "m328pdef.inc"		; definiciones del atmega

.org 0x0000					; para comenzar el programa en la direccion reset

main:

	LDI R16, 15				; R16 = A = 5
	LDI R17, 1				; R17 = B = 3
	LDI R18, 2				; R18 = S = 2 , es la operacion ADD


	CPI R18, 0				; Compara S con 0
	BREQ OP_CLEAR			; Si si vale 0 salta a OP_CLEAR

	CPI R18, 1				; Compara S con 1
	BREQ OP_SUB				; Si si vale 1 salta a OP_SUB

	CPI R18, 2
	BREQ OP_ADD

	CPI R18, 3
	BREQ OP_XOR

	CPI R18, 4
	BREQ OP_AND

	CPI R18, 5
	BREQ OP_OR

	CPI R18, 6
	BREQ OP_SHL

	CPI R18, 7
	BREQ OP_INC



loop:
	RJMP loop



OP_CLEAR:
	CLR R19					; Pone el registro en 0
	RJMP FLAGS

OP_SUB:
	MOV R19, R16			; Copio A en R19 para no cambiar el valor inicial de R16
	SUB R19, R17			; R19 = A - B
	RJMP FLAGS

OP_ADD:
	MOV R19, R16
	ADD R19, R17			; R19 = A + B
	RJMP FLAGS

OP_XOR:
	MOV R19, R16
	EOR R19, R17			; R19 = A XOR B
	RJMP FLAGS

OP_AND:
    MOV R19, R16
    AND R19, R17			; R19 = A * B
    RJMP FLAGS

OP_OR:
    MOV R19, R16
    OR R19, R17				; R19 = A + B
    RJMP FLAGS

OP_SHL:
    MOV R19, R16
    LSL R19					; R19 desplaza un bit a la izquierda, 0101 pasa a 1010
    RJMP FLAGS

OP_INC:
    MOV R19, R16
    INC R19					; R19 incrementa 1
    RJMP FLAGS



	; BANDERAS DE LA ALU
	; R20 = 00000NZC



FLAGS:
    CLR R20					; Limpia el registro de banderas (C, N y Z = 0)



	;CARRY:


	CPI R18, 1						; La operación fue RESTA?
	BREQ CARRY_RESTA				; Si fue resta, revisa si hubo préstamo / CARRY_RESTA es especial porque se necesita saber si hubo prestamo

	CPI R18, 2                      ; La operación fue SUMA?
    BREQ CARRY_BIT4                 ; Si fue suma, revisa el bit 4 del resultado para ver si hubo carry

	CPI R18, 6                      ; La operación fue SHL?
    BREQ CARRY_BIT4                 ; Si fue SHL, revisa el bit 4 del resultado

	CPI R18, 7                      ; La operación fue INC?
    BREQ CARRY_BIT4                 ; Si fue INC, revisa el bit 4 del resultado


	RJMP RESULTADO_4_BITS           ; CLEAR, XOR, AND y OR no generan Carry


	
CARRY_RESTA:
	CP R16, R17                     ; Compara A con B sin modificar 
    BRCC RESULTADO_4_BITS           ; Si A >= B, no hubo préstamo y C queda en 0

    ORI R20, 0b00000001             ; Si A < B, hubo préstamo entonces C = 1
    RJMP RESULTADO_4_BITS           ; Continúa con el resultado de 4 bits



CARRY_BIT4:
    SBRC R19, 4                     ; Si el bit 4 de R19 es 0, salta la siguiente línea
    ORI R20, 0b00000001             ; Si el bit 4 es 1 entonces C = 1

    RJMP RESULTADO_4_BITS           ; Continúa con el resultado de 4 bits


RESULTADO_4_BITS:
    ANDI R19, 0b00001111            ; Conserva solo los bits 3,2,1,0 de R19



ZERO:
    TST R19                         ; Comprueba si el resultado R19 es cero
    BRNE NEGATIVO                   ; Si no es cero, pasa a comprobar Negativo

    ORI R20, 0b00000010             ; Si R19 = 0 -> Z = 1 (bit 1 de R20)


NEGATIVO:
    SBRC R19, 3                     ; Si el bit 3 de R19 es 0, salta la siguiente línea
    ORI R20, 0b00000100             ; Si el bit 3 es 1 -> N = 1 (bit 2 de R20)


	RJMP loop                       ; Termina la prueba y queda en el loop
