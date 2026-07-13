#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <emscripten.h>

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

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
    u16 stack[STACK_SIZE];
    u8 sp;
    u16 pc;

    u8 display[64 * 32];
} Chip8;

typedef struct {
    u8 redraw;
    u8 waiting;
} Chip8_Flags;

Chip8 *chip8_init();
int chip8_load(Chip8 *chip8, char *path);
Chip8_Flags chip8_execute(Chip8 *chip8, u8 *keys);

static Chip8 *chip8;
static u32 timestamp;
static u8 keys[17];
static u8 waiting;
static i32 waiting_key = -1;

#define QUIRK_VF_RESET 0x1
#define QUIRK_MEMORY 0x2
#define QUIRK_SHIFTING 0x4

static const u32 quirks = QUIRK_VF_RESET | QUIRK_MEMORY | QUIRK_SHIFTING;

SDL_AppResult SDL_AppInit (void **appstate, int argc, char *argv[]) {
    // if (argc < 2) {
    //     fprintf(stderr, "Usage: c8 <rom>");
    //     exit(1);
    // }

    chip8 = chip8_init();
    if (-1 == chip8_load(chip8, "./roms/down8.ch8")) {
        return -1;
    }

    if (-1 == init_screen()) {
        return SDL_APP_FAILURE;
    }

    clear_screen();
    timestamp = millis();
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate (void *appstate) {
    Chip8_Flags flags;
    for (int i = 0; i < INST_PER_FRAME; ++i) {
        Chip8_Flags i_flags = chip8_execute(chip8, keys);
        flags.redraw |= i_flags.redraw;
        if (waiting) {
            break;
        }
    }

    // process sound 

    if (millis() - timestamp >= (1000/60)) {
        timestamp = millis();
        if (chip8->DT > 0)
            chip8->DT--;
        if (chip8->ST > 0)
            chip8->ST--;
    }

    if (chip8->ST > 0) {
        play_tone();
    } else {
        pause_tone();
    }

    if (flags.redraw)
        update_graphics(chip8->display);
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    if (event->type == SDL_EVENT_KEY_UP && waiting) {
        int key_released = scancode_to_chip8(event->key.scancode);
        if (key_released != -1) {
            waiting = 0;
            waiting_key = key_released;
        }
    }
    get_keys(keys);
    if (keys[16]) {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    free(chip8);
}

// Returns new chip8 state object. Caller owns object
Chip8 *chip8_init() {
    Chip8 *new = malloc(sizeof(Chip8));
    memset(new, 0, sizeof(Chip8));
    memcpy(new->memory, fonts, sizeof(fonts));
    new->pc = 0x200;
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

Chip8_Flags chip8_execute(Chip8 *chip8, u8 *keys) {
    if (waiting) {
        return (Chip8_Flags){0};
    }
    Chip8_Flags flags = {0};
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
                    flags.redraw = 1;
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
                            u8 flag = chip8->V[x] <= chip8->V[y];
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
                      flags.redraw = 1;
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
                              if (waiting_key == -1) {
                                  waiting = 1;
                                  incr_pc = 0;
                                  break;
                              }
                              if (!waiting) {
                                  chip8->V[x] = waiting_key;
                                  waiting_key = -1;
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
                  // mvprintw(40, 0, "unknown opcode %x", inst);
                  // uninit_screen();
                  // exit(1);
                  break;
    }
    if (incr_pc)
        chip8->pc += 2;
    return flags;
}
