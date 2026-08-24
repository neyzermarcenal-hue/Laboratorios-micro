.include "m328pdef.inc"		; definiciones del atmega

.org 0x0000					; para comenzar el programa en la direccion reset

main:

	LDI R16, 5				; R16 = A = 5
	LDI R17, 3				; R17 = B = 3
	LDI R18, 6				; R18 = S = 2 , es la operacion ADD


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
	RJMP loop				; Termina la operacion y va al loop

OP_SUB:
	MOV R19, R16			; Copio A en R19 para no cambiar el valor inicial de R16
	SUB R19, R17			; R19 = A - B
	RJMP loop

OP_ADD:
	MOV R19, R16
	ADD R19, R17			; R19 = A + B
	RJMP loop

OP_XOR:
	MOV R19, R16
	EOR R19, R17			; R19 = A XOR B
	RJMP loop

OP_AND:
    MOV R19, R16
    AND R19, R17			; R19 = A * B
    RJMP loop

OP_OR:
    MOV R19, R16
    OR R19, R17				; R19 = A + B
    RJMP loop

OP_SHL:
    MOV R19, R16
    LSL R19					; R19 desplaza un bit a la izquierda, 0101 pasa a 1010
    RJMP loop

OP_INC:
    MOV R19, R16
    INC R19					; R19 incrementa 1
    RJMP loop