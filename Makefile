chip8: main.c isa.h screen.c screen.h
	gcc -Wall -Wextra -Wpedantic -g -lncurses main.c screen.c -o chip8

html: 
	emcc -Wall -Wextra -g -sUSE_SDL=3 clear.c -o clear.html

gdb: chip8
	gdb -x cmds.gdb --tui ./chip8

tests: infloop.S
	xxd -r -p infloop.S infloop.ch8

clean:
	rm chip8.html chip8.js chip8.wasm
	rm chip8
