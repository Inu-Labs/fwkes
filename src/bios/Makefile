.PHONY: all clean

all: bios.nes

bios.nes: main.o crt0.o nes.cfg
	ld65 -C nes.cfg -o bios.nes crt0.o main.o nes.lib

crt0.o: crt0.s
	ca65 -I ../../include/fwkes/bios crt0.s

main.o: main.s
	ca65 main.s

main.s: main.c
	cc65 -I ../../include/fwkes/bios -I ../../include --standard cc65 --add-source -Oi main.c -o main.s

clean:
	rm -f main.o main.s crt0.o bios.nes
