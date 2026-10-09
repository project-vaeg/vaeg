
	ORG		0
	USE16
	CPU		8086

FIXCS		equ		2eh

segment .bios start=0x0000

				jmp		short biosdispatch
				jmp		short biosstart
				db		0, 0, 0, 0

biosstart:		cli
				xor		ax, ax
				mov		ds, ax
				mov		al, 30h
				mov		ss, ax
				mov		sp, 00feh
				jmp		biosmain

biosdispatch:	cli
				xor		ax, ax
				mov		ds, ax
				in		al, 35h
				test	al, 80h
				jne		short biosdispitf
				mov		ss, [0x0406]
				mov		sp, [0x0404]
				retf

biosdispitf:	test	al, 20h
				je		short biosstart
				call	itfcall
itfcall_ret:	jc		short biosstart
				db		0eah
				dw		04f8h
				dw		0000h


section	.table start=0x040

		dw		biosnop		; 00
		dw		biosnop		; 01
		dw		vect02		; 02
		dw		biosnop		; 03
		dw		biosnop		; 04
		dw		biosnop		; 05
		dw		biosnop		; 06
		dw		biosnop		; 07

		dw		vect08		; 08
		dw		vect09		; 09
		dw		sendeoi		; 0a
		dw		sendeoi		; 0b
		dw		bios0c		; 0c
		dw		sendeoi		; 0d
		dw		sendeoi		; 0e
		dw		sendeoi		; 0f

		dw		sendeoi		; 10
		dw		sendeoi		; 11
		dw		bios12		; 12
		dw		vect13		; 13
		dw		sendeoi		; 14
		dw		sendeoi		; 15
		dw		sendeoi		; 16
		dw		sendeoi		; 17

		dw		biosnop		; 18
		dw		biosnop		; 19
		dw		biosnop		; 1a
		dw		biosnop		; 1b
		dw		biosnop		; 1c
		dw		biosnop		; 1d
		dw		biosnop		; 1e
		dw		biosnop		; 1f


section	.biosmain start=0x0080

				align	4
itfcall:		nop						; 80
				jmp		itfcall_ret

				align	4
biosinit:		nop						; 84
				jmp		biosmain2

				align	4
bios09:			nop						; 88
				ret

				align	4
bios0c:			nop						; 8c
				iret

				align	4
bios12:			nop						; 90
				iret

				align	4
bios13:			nop						; 94
				iret

				; INT18h-INT1Fh are fixed default-IRET slots. Keep the
				; reserved space so BIOSOFST_WAIT remains at 00b4.
				times 0x1e nop

biosfdcwait:	nop						; b4
				iret

%include	'biosmain.x86'
%include	'eoi.x86'
%include	'vect02.x86'
%include	'vect08.x86'
%include	'vect09.x86'
%include	'vect13.x86'

	ends

