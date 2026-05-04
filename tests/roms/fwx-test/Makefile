.PHONY: all

all: fwx-test.nes

fwx-test.nes: main.o reset.o nes.cfg
	ld65 -C nes.cfg -o fwx-test.nes reset.o main.o nes.lib

reset.o: reset.s
	ca65 reset.s -o reset.o

main.o: main.s
	ca65 main.s

main.s: main.c
	cc65 --standard cc65 --add-source -Oi main.c -o main.s

clean:
	rm -f main.o main.s reset.o fwx-test.nes
