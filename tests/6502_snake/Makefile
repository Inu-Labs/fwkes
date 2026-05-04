OBJS := main.o

.PHONY: all clean

all: 6502_snake.nes

clean:
	rm $(OBJS) 6502_snake.nes

%.o: %.s
	ca65 $< -o $@

main.o: main.s nesdefs.s

6502_snake.nes: $(OBJS)
	ld65 -C link_script.cfg $(OBJS) -o $@
