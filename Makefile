chip8: main.c isa.h screen.c screen.h
	gcc -Wall -Wextra -Wpedantic -g -lncurses main.c screen.c -o chip8

gdb: chip8
	gdb -x cmds.gdb --tui ./chip8

tests: infloop.S
	xxd -r -p infloop.S infloop.ch8
