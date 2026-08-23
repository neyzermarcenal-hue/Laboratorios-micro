.include "m328pdef.inc"

.org 0x0000

main:

	LDI R16, 5
	LDI R17, 3

	ADD R16, R17


loop:
	RJMP loop