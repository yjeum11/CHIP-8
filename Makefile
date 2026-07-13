SRC=src/*.c

html: roms/buzz.ch8 src/main.c src/screen.c
	emcc -Wall -Wextra -sUSE_SDL=3 -I./include/ --embed-file ./roms/ $(SRC) -o chip8.html

roms/buzz.ch8: roms/buzz.hex
	xxd -r -p roms/buzz.hex > roms/buzz.ch8

gdb: chip8
	gdb -x cmds.gdb --tui ./chip8

tests: infloop.S
	xxd -r -p infloop.S infloop.ch8

clean:
	rm chip8.html chip8.js chip8.wasm
	rm chip8
