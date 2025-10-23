		org	$e000

data		fcb	$fa,$ce

handle_reset	lda	#$42
		lde	#$de
		ldf	#$ad
		ldw	data
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
