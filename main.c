#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "util.h"
#include "isa.h"
#include "screen.h"

const u8 fonts[16*5] = {
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

typedef struct Chip8 {
    u8 V [NUM_REGS];
    u16 I;
    u8 DT;
    u8 ST;

    u8 memory[MEM_SIZE];
    u8 stack[STACK_SIZE];
    u8 sp;
    u16 pc;

    u8 display[64 * 32];
} Chip8;

// Returns new chip8 state object. Caller owns object
Chip8 *chip8_init() {
    Chip8 *new = malloc(sizeof(Chip8));
    memset(new, 0, sizeof(Chip8));
    memcpy(new->memory, fonts, sizeof(fonts));
    return new;
}

int chip8_load(Chip8 *chip8, char *path) {
    FILE *prog_f = fopen(path, "rb");
    if (prog_f == NULL) {
        perror("Loading program failed");
        return -1;
    }
    fread(chip8->memory+0x200, 1, 4096, prog_f);
    fclose(prog_f);
    return 0;
}

void chip8_execute(Chip8 *chip8) {
    u8 incr_pc = 1;
    u16 inst = chip8->memory[chip8->pc] << 8 | chip8->memory[chip8->pc+1];
    u16 nnn = inst & 0x0FFF;
    u8 x = (inst & 0x0F00) >> 8;
    u8 y = (inst & 0x00F0) >> 4;
    u8 n = inst & 0x000F;
    u16 kk = inst & 0x00FF;
    u8 code = (inst & 0xF000) >> 12;
    switch (code) {
        case 0:
            switch (inst) {
                case CLS:
                    clear_screen();
                    break;
                case RET:
                    chip8->pc = chip8->stack[--chip8->sp];
                    incr_pc = 0;
                    break;
            }
            break;
        case 1: // Jump to nnn
            chip8->pc = nnn;
            incr_pc = 0;
            break;
        case 2: // Call subroutine nnn
            chip8->stack[chip8->sp++] = chip8->pc+2;
            chip8->pc = nnn;
            incr_pc = 0;
            break;
        case 3: // Skip if x == kk
            if (chip8->V[x] == kk)
                chip8->pc += 2;
            break;
        case 4: // Skip if x != kk
            if (chip8->V[x] != kk)
                chip8->pc += 2;
            break;
        case 5:
            if (chip8->V[x] == chip8->V[y])
                chip8->pc += 2;
            break;
        case 6:
            chip8->V[x] = kk;
            break;
        case 7:
            chip8->V[x] += kk;
            break;
        case 8:
            switch (inst & 0xF) {
                case 0:
                    chip8->V[x] = chip8->V[y];
                    break;
                case 1:
                    chip8->V[x] |= chip8->V[y];
                    break;
                case 2:
                    chip8->V[x] &= chip8->V[y];
                    break;
                case 3:
                    chip8->V[x] ^= chip8->V[y];
                    break;
                case 4:
                    ;
                    u8 carry = (((u16)chip8->V[x] + (u16)chip8->V[y]) & 0x100) >> 8;
                    chip8->V[x] += chip8->V[y];
                    chip8->V[0xF] = carry;
                    break;
                case 5: {
                            u8 flag = chip8->V[x] >= chip8->V[y];
                            chip8->V[x] = chip8->V[x] - chip8->V[y];
                            chip8->V[0xF] = flag;
                            break;
                        }
                case 6: {
                            u8 flag = chip8->V[x] & (0x01);
                            chip8->V[x] >>= 1;
                            chip8->V[0xF] = flag;
                            break;
                        }
                case 7:
                        chip8->V[x] = chip8->V[y] - chip8->V[x];
                        chip8->V[0xF] = chip8->V[x] < chip8->V[y];
                        break;
                case 0xE: {
                              u8 flag = !!(chip8->V[x] & 0x80); // trick to detect one in MSB
                              chip8->V[x] <<= 1;
                              chip8->V[0xF] = flag;
                              break;
                          }

            }
            break;
        case 9:
            if (chip8->V[x] != chip8->V[y])
                chip8->pc += 2;
            break;
        case 0xA:
            chip8->I = nnn;
            break;
        case 0xB:
            chip8->pc = nnn + chip8->V[0];
            incr_pc = 0;
            break;
        case 0xC:
            ;
            u8 randint = rand();
            chip8->V[x] = randint & kk;
            break;
        case 0xD: {
                      u8 sprite[15] = {0};
                      for (int i = 0; i < n; ++i) {
                          sprite[i] = chip8->memory[chip8->I+i];
                      }
                      chip8->V[0xF] = draw_sprite(chip8->display, sprite, chip8->V[x], chip8->V[y]);
                      break;
                  }
        case 0xE: {
                      i16 key = get_key();
                      u8 c8key = keyboard_to_chip8(key);
                      if (kk == 0x9E) {
                          if (chip8->V[x] == keyboard_to_chip8(key)) {
                              chip8->pc += 2;
                          }
                      } else if (kk == 0xA1) {
                          if (chip8->V[x] != keyboard_to_chip8(key)) {
                              chip8->pc += 2;
                          }
                      }
                      break;
                  }
        case 0xF: {
                      switch (kk) {
                          case 0x07:
                              chip8->V[x] = chip8->DT;
                              break;
                          case 0x0A:
                              chip8->V[x] = keyboard_to_chip8(get_key_block());
                              break;
                          case 0x15:
                              chip8->DT = chip8->V[x];
                              break;
                          case 0x18:
                              chip8->ST = chip8->V[x];
                              break;
                          case 0x1E:
                              chip8->I += chip8->V[x];
                              break;
                          case 0x29:
                              // assuming fonts start at 0x0
                              chip8->I = chip8->V[x] * 5 + 0x0;
                              break;
                          case 0x33: {
                                         u8 hund = chip8->V[x] / 100;
                                         u8 tens = (chip8->V[x] % 100) / 10;
                                         u8 ones = chip8->V[x] % 10;
                                         chip8->memory[chip8->I] = hund;
                                         chip8->memory[chip8->I+1] = tens;
                                         chip8->memory[chip8->I+2] = ones;
                                         break;
                                     }
                          case 0x55:
                                     for (int i = 0; i <= x; ++i) {
                                         chip8->memory[chip8->I+i] = chip8->V[i];
                                     }
                                     break;
                          case 0x65:
                                     for (int i = 0; i <= x; ++i) {
                                         chip8->V[i] = chip8->memory[chip8->I+i];
                                     }
                                     break;
                      }
                      break;
                  }
        default:
                  // mvprintw(40, 0, "unknown opcode %x", inst);
                  // uninit_screen();
                  // exit(1);
                  break;
    }
    if (incr_pc)
        chip8->pc += 2;
}

int main (int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: c8 <rom>");
        exit(1);
    }

    Chip8 *chip8 = chip8_init();
    if (-1 == chip8_load(chip8, argv[1])) {
        return -1;
    }

    init_screen();

    struct timespec ts;

    u32 timestamp = millis(&ts);
    i32 *inputs = malloc(sizeof(i32) * 6);
    for (;;) {
        // get input
        get_input(inputs);
        chip8_execute(chip8, inputs);
        if (millis(&ts) - timestamp >= (1000/60)) {
            timestamp = millis(&ts);
            if (chip8->DT > 0)
                chip8->DT--;
            if (chip8->ST > 0)
                chip8->ST--;
        }
        update_graphics(chip8->display);
        if (get_key() == 'p') {
            uninit_screen();
            exit(0);
        }
    }

}


