OBJS := main.o

.PHONY: all clean

all: main.nes

clean:
	rm $(OBJS) main.nes

%.o: %.s
	ca65 $< -o $@

main.o: main.s nesdefs.s

main.nes: $(OBJS)
	ld65 -C link_script.cfg $(OBJS) -o $@
