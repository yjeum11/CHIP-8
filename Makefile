SRC=main.c screen.c

chip8: main.c isa.h screen.c screen.h
	gcc -Wall -Wextra -Wpedantic -g -lncurses $(SRC) -o chip8

html: 
	emcc -Wall -Wextra -sUSE_SDL=3 --embed-file ./roms/ $(SRC) -o chip8.html

gdb: chip8
	gdb -x cmds.gdb --tui ./chip8

tests: infloop.S
	xxd -r -p infloop.S infloop.ch8

clean:
	rm chip8.html chip8.js chip8.wasm
	rm chip8
