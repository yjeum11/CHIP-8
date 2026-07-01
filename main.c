#include <curses.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "isa.h"
#include "screen.h"

const uint8_t fonts[16*5] = {
    0xf0, 0x90, 0x90, 0x90, 0xf0,
    0x20, 0x60, 0x20, 0x20, 0x70,
    0xF0, 0x10, 0xF0, 0x80, 0xF0,
    0xF0, 0x10, 0xF0, 0x10, 0xF0,
    0x90, 0x90, 0xF0, 0x10, 0x10,
    0xF0, 0x80, 0xF0, 0x10, 0xF0,
    0xF0, 0x80, 0xF0, 0x90, 0xF0,
    0xF0, 0x10, 0x20, 0x40, 0x40,
    0xF0, 0x90, 0xF0, 0x90, 0xF0,
    0xF0, 0x90, 0xF0, 0x10, 0xF0,
    0xF0, 0x90, 0xF0, 0x90, 0x90,
    0xE0, 0x90, 0xE0, 0x90, 0xE0,
    0xF0, 0x80, 0x80, 0x80, 0xF0,
    0xE0, 0x90, 0x90, 0x90, 0xE0,
    0xF0, 0x80, 0xF0, 0x80, 0xF0,
    0xF0, 0x80, 0xF0, 0x80, 0xF0
};

int main (int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: c8 <rom>");
        exit(1);
    }
    // general-purpose registers
    uint8_t V[NUM_REGS];
    // index register
    uint16_t I = 0;
    // delay timer, sound timer. should automatically decrement at 60Hz
    uint8_t DT = 0;
    uint8_t ST = 0;

    uint8_t memory[MEM_SIZE];
    uint16_t stack[STACK_SIZE];
    uint8_t sp = 0;
    uint16_t pc = RESET_PC;

    uint8_t display[64 * 32];

    memset(V, 0, 16);
    memset(memory, 0, 4096);
    memset(display, 0, 64 * 32);
    memset(stack, 0, 16*sizeof(uint16_t));

    memcpy(memory, fonts, sizeof(fonts));

    // Load program
    FILE *prog_f = fopen(argv[1], "rb");
    fread(memory+0x200, 1, 4096, prog_f);
    fclose(prog_f);

    FILE *out_tty = fopen("/dev/pts/7", "w");

    memory[0x1ff] = 1;

    init_screen();

    struct timespec ts;

    uint32_t timestamp = millis(&ts);
    uint8_t incr_pc = 1;
    for (;;) {
        incr_pc = 1;
        uint16_t inst = memory[pc] << 8 | memory[pc+1];
        uint16_t nnn = inst & 0x0FFF;
        uint8_t x = (inst & 0x0F00) >> 8;
        uint8_t y = (inst & 0x00F0) >> 4;
        uint8_t n = inst & 0x000F;
        uint16_t kk = inst & 0x00FF;
        uint8_t code = (inst & 0xF000) >> 12;
        switch (code) {
            case 0:
                switch (inst) {
                    case CLS:
                        clear_screen();
                        break;
                    case RET:
                        pc = stack[--sp];
                        incr_pc = 0;
                        break;
                }
                break;
            case 1: // Jump to nnn
                pc = nnn;
                incr_pc = 0;
                break;
            case 2: // Call subroutine nnn
                stack[sp++] = pc+2;
                pc = nnn;
                incr_pc = 0;
                break;
            case 3: // Skip if x == kk
                if (V[x] == kk)
                    pc += 2;
                break;
            case 4: // Skip if x != kk
                if (V[x] != kk)
                    pc += 2;
                break;
            case 5:
                if (V[x] == V[y])
                    pc += 2;
                break;
            case 6:
                V[x] = kk;
                break;
            case 7:
                V[x] += kk;
                break;
            case 8:
                switch (inst & 0xF) {
                    case 0:
                        V[x] = V[y];
                        break;
                    case 1:
                        V[x] |= V[y];
                        break;
                    case 2:
                        V[x] &= V[y];
                        break;
                    case 3:
                        V[x] ^= V[y];
                        break;
                    case 4:
                        ;
                        uint8_t carry = (((uint16_t)V[x] + (uint16_t)V[y]) & 0x100) >> 8;
                        V[x] += V[y];
                        V[0xF] = carry;
                        break;
                    case 5: {
                        uint8_t flag = V[x] >= V[y];
                        V[x] = V[x] - V[y];
                        V[0xF] = flag;
                        break;
                    }
                    case 6: {
                        uint8_t flag = V[x] & (0x01);
                        V[x] >>= 1;
                        V[0xF] = flag;
                        break;
                    }
                    case 7:
                        V[x] = V[y] - V[x];
                        V[0xF] = V[x] < V[y];
                        break;
                    case 0xE: {
                        uint8_t flag = !!(V[x] & 0x80); // trick to detect one in MSB
                        V[x] <<= 1;
                        V[0xF] = flag;
                        break;
                    }

                }
                break;
            case 9:
                if (V[x] != V[y])
                    pc += 2;
                break;
            case 0xA:
                I = nnn;
                break;
            case 0xB:
                pc = nnn + V[0];
                incr_pc = 0;
                break;
            case 0xC:
                ;
                uint8_t randint = rand();
                V[x] = randint & kk;
                break;
            case 0xD:
                ;
                uint8_t sprite[15] = {0};
                for (int i = 0; i < n; ++i) {
                    sprite[i] = memory[I+i];
                }
                V[0xF] = draw_sprite(display, sprite, V[x], V[y]);
                break;
            case 0xE:
                ;
                int16_t key = get_key();
                uint8_t c8key = keyboard_to_chip8(key);
                if (kk == 0x9E) {
                    if (V[x] == keyboard_to_chip8(key)) {
                        pc += 2;
                    }
                } else if (kk == 0xA1) {
                    if (V[x] != keyboard_to_chip8(key)) {
                        pc += 2;
                    }
                }
                break;
            case 0xF:
                switch (kk) {
                    case 0x07:
                        V[x] = DT;
                        break;
                    case 0x0A:
                        V[x] = keyboard_to_chip8(get_key_block());
                        break;
                    case 0x15:
                        DT = V[x];
                        break;
                    case 0x18:
                        ST = V[x];
                        break;
                    case 0x1E:
                        I += V[x];
                        break;
                    case 0x29:
                        // assuming fonts start at 0x0
                        I = V[x] * 5 + 0x0;
                        break;
                    case 0x33:
                        ;
                        uint8_t hund = V[x] / 100;
                        uint8_t tens = (V[x] % 100) / 10;
                        uint8_t ones = V[x] % 10;
                        memory[I] = hund;
                        memory[I+1] = tens;
                        memory[I+2] = ones;
                        break;
                    case 0x55:
                        for (int i = 0; i <= x; ++i) {
                            memory[I+i] = V[i];
                        }
                        break;
                    case 0x65:
                        for (int i = 0; i <= x; ++i) {
                            V[i] = memory[I+i];
                        }
                        break;
                }
                break;
            default:
                mvprintw(40, 0, "unknown opcode %x", inst);
                // uninit_screen();
                // exit(1);
                break;
        }
        if (incr_pc)
            pc += 2;
        if (millis(&ts) - timestamp >= (1000/60)) {
            timestamp = millis(&ts);
            if (DT > 0)
                DT--;
            if (ST > 0)
                ST--;
        }
        update_graphics(display);
        if (get_key() == 'p') {
            fclose(out_tty);
            uninit_screen();
            exit(0);
        }
    }

}


