#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "chip8.h"

// Returns new chip8 state object. Caller owns object
Chip8 *chip8_init() {
    Chip8 *new = malloc(sizeof(Chip8));
    memset(new, 0, sizeof(Chip8));
    memcpy(new->memory, fonts, sizeof(fonts));
    new->pc = 0x200;
    return new;
}

void chip8_reset(Chip8 *chip8) {
    memset(chip8, 0, sizeof(Chip8));
    memcpy(chip8->memory, fonts, sizeof(fonts));
    chip8->pc = 0x200;
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

Chip8_State chip8_execute(Chip8 *chip8, u8 *keys, Chip8_State state) {
    if (state.waiting) {
        return state;
    }
    Chip8_State next_state = {0};
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
                    memset(chip8->display, 0, sizeof(chip8->display));
                    next_state.redraw = 1;
                    break;
                case RET:
                    chip8->sp--;
                    chip8->pc = chip8->stack[chip8->sp];
                    incr_pc = 0;
                    break;
            }
            break;
        case 1: // Jump to nnn
            chip8->pc = nnn;
            incr_pc = 0;
            break;
        case 2: // Call subroutine nnn
            chip8->stack[chip8->sp] = chip8->pc+2;
            chip8->sp++;
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
                    if (quirks & QUIRK_VF_RESET) {
                        chip8->V[0xF] = 0;
                    }
                    break;
                case 2:
                    chip8->V[x] &= chip8->V[y];
                    if (quirks & QUIRK_VF_RESET) {
                        chip8->V[0xF] = 0;
                    }
                    break;
                case 3:
                    chip8->V[x] ^= chip8->V[y];
                    if (quirks & QUIRK_VF_RESET) {
                        chip8->V[0xF] = 0;
                    }
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
                            if (quirks & QUIRK_SHIFTING) {
                                chip8->V[x] = chip8->V[y];
                            }
                            u8 flag = chip8->V[x] & (0x01);
                            chip8->V[x] >>= 1;
                            chip8->V[0xF] = flag;
                            break;
                        }
                case 7:
                        ;
                        u8 notborrow = chip8->V[y] >= chip8->V[x];
                        chip8->V[x] = chip8->V[y] - chip8->V[x];
                        chip8->V[0xF] = notborrow;
                        break;
                case 0xE: {
                              if (quirks & QUIRK_SHIFTING) {
                                  chip8->V[x] = chip8->V[y];
                              }
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
        case 0xC: {
            u8 randint = rand();
            chip8->V[x] = randint & kk;
            break;
        }
        case 0xD: {
                      u8 sprite[15] = {0};
                      for (int i = 0; i < n; ++i) {
                          sprite[i] = chip8->memory[chip8->I+i];
                      }
                      chip8->V[0xF] = draw_sprite(chip8->display, sprite, chip8->V[x], chip8->V[y]);
                      next_state.redraw = 1;
                      break;
                  }
        case 0xE: {
                      if (kk == 0x9E) {
                          if (keys[chip8->V[x]]) {
                              chip8->pc += 2;
                          }
                      } else if (kk == 0xA1) {
                          if (!keys[chip8->V[x]]) {
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
                              if (state.waiting_key == -1) {
                                  next_state.waiting = 1;
                                  incr_pc = 0;
                                  break;
                              }
                              if (!state.waiting) {
                                  chip8->V[x] = state.waiting_key;
                                  next_state.waiting_key = -1;
                                  incr_pc = 1;
                                  break;
                              }
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
                                     if (quirks & QUIRK_MEMORY) {
                                        chip8->I += x+1;
                                     }
                                     break;
                          case 0x65:
                                     for (int i = 0; i <= x; ++i) {
                                         chip8->V[i] = chip8->memory[chip8->I+i];
                                     }
                                     if (quirks & QUIRK_MEMORY) {
                                        chip8->I += x+1;
                                     }
                                     break;
                      }
                      break;
                  }
        default:
                  break;
    }
    if (incr_pc)
        chip8->pc += 2;
    return next_state;
}

uint8_t draw_sprite(uint8_t *screen, uint8_t *sprite, uint8_t x, uint8_t y) {
    int flipped = 0;
    for (int i = 0; i < 15; ++i) {
        for (int j = 0; j < 8; ++j) {
            uint8_t curr = (sprite[i] >> (7-j)) & 1;
            int wrapi = (y + i) % 32;
            int wrapj = (x + j) % 64;
            if (screen[wrapi*64+wrapj] && curr)
                flipped = 1;
            screen[wrapi*64+wrapj] = (screen[wrapi*64+wrapj] ^ curr) & 1;
        }
    }
    return flipped;
}

