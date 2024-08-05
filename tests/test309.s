		org	$e000

data		fcb	$fa,$ce,$b0,$0c,$5a,$a5

handle_reset	lda	#$42
		lde	#$de
		ldf	#$ad
		ldw	data
		ldq	#$0fedf00d
		
		ldu	#$2000
		lds	#$1000

		leax	data,pcr
		ldq	2,x
		ldq	,x

		stq	5,x
		ldw	5,x

		lda	#40
		lde	#2
		addr	a,e

		ldd	#$dead
		ldx	#3
		addr	d,x

		lda	#4
		addr	a,u

		sync

nohandler	rti

		org	$fff0
		fdb	nohandler	; $fff0
		fdb	nohandler	; $fff2
		fdb	nohandler	; $fff4
		fdb	nohandler	; $fff6
		fdb	nohandler	; $fff8
		fdb	nohandler	; $fffa
		fdb	nohandler	; $fffc
		fdb	handle_reset	; $fffe

		end	handle_reset
