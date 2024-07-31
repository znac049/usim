uart		equ	$c000
system_stack	equ	$0200
user_stack	equ	$0400
vector_table	equ	$fff0
rom_start	equ	$e000

acia		equ	$c000

inchar		equ	$00

; mc6850 Uart registers
MC6850.StatusReg equ	0
MC6850.DataReg	equ	1

YES		equ	1
OK		equ	1
NO		equ	0
ERR		equ	0


; divide by 16 yields 115200 with system clock of 7.37280MHz. Master Reset.
MC6850.InitialCR equ	$95			


;		setdp	$00

;
;		Start of System ROM
;
		org	rom_start

handle_reset	lds	#system_stack
		ldu	#user_stack
		orcc	#$50		; disable interrupts
		lbsr	serialInit

		leax    system_ready,pcr
		lbsr    putstr

zob		bra	zob


serialInit	pshs	x
		ldx	#uart
		lda	#$03			; Master reset
		sta	MC6850.StatusReg,x

; After a Master reset, the tdre bit should be set
		lda	MC6850.StatusReg,x
		cmpa	#$ff
		beq	siNoDevice

; Complete the initialisation
		lda	#MC6850.InitialCR
		sta	MC6850.StatusReg,x
		lda	#OK
		bra	siDone

siNoDevice	lda	#ERR

siDone		puls	x,pc


putChar		pshs	b,x
		ldx	#uart
spcWait		ldb	MC6850.StatusReg,x
		bitb	#2			; tx register empty?
		beq	spcWait
		
		sta	MC6850.DataReg,x	; send the char

		puls	b,x,pc


getChar		pshs	x
		ldx	#uart
sgcWait		lda	MC6850.StatusReg,x
		bita	#1			; rx register full?
		beq	sgcWait

		lda	MC6850.DataReg,x	; grab the character

		puls	x,pc



putstr		pshs	a,x,cc
putstr_loop	lda	,x+
		beq	putstr_done
		bsr	putChar
		bra	putstr_loop
putstr_done	puls	a,x,cc
		rts


system_ready	fcc	"System loaded and ready"
		fcb 	13,10+$80

handle_irq	rti

handle_firq	rti

handle_undef	rti

handle_swi	rti

handle_swi2	rti

handle_swi3	rti

handle_nmi	rti

;
;		System vector specification
;
		org	vector_table
		fdb	handle_undef	; $fff0
		fdb	handle_swi3	; $fff2
		fdb	handle_swi2	; $fff4
		fdb	handle_firq	; $fff6
		fdb	handle_irq	; $fff8
		fdb	handle_swi	; $fffa
		fdb	handle_nmi	; $fffc
		fdb	handle_reset	; $fffe

		end	handle_reset
