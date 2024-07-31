.PHONY: all clean

%.decb: %.asm
	asm6809 --coco --6809 --listing=$*.lst --output=$@ $<

%.decb: %.s
	asm6809 --coco --6809 --listing=$*.lst --output=$@ $<

all: test309.decb

clean:
	rm -f *.hex *.srec *.decb *.lst *~