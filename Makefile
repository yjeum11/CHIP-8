SRC=$(wildcard src/*.c)

html: chip8.html

chip8.html: $(SRC)
	emcc -Wall -Wextra -sUSE_SDL=3 -I./include/ --embed-file ./roms/ $(SRC) -o chip8.html

gdb: chip8
	gdb -x cmds.gdb --tui ./chip8

tests: infloop.S
	xxd -r -p infloop.S infloop.ch8

clean:
	rm chip8.html chip8.js chip8.wasm
	rm chip8
