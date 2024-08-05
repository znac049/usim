.PHONY: all clean

%.decb: %.asm
	asm6809 --coco --6309 --listing=$*.lst --output=$@ $<

%.decb: %.s
	asm6809 --coco --6309 --listing=$*.lst --output=$@ $<

all: test309.decb single_instruction.decb

clean:
	rm -f *.hex *.srec *.decb *.lst *~