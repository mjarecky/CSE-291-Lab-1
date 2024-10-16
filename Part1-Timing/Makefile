include ../cpu.mk

all: main main-visual

run: main
	@taskset -c $(SENDER_CPU) ./main

run-visual: main-visual
	@taskset -c $(SENDER_CPU) ./main-visual

main: main.c Makefile
	@gcc main.c -O0 -o main

main-visual: main.c Makefile
	@gcc main.c -O0 -DVISUAL -o main-visual

.PHONY: clean

clean:
	rm -f main
