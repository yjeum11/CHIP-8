#ifndef SCREEN_H
#define SCREEN_H

#include "util.h"
#include <SDL3/SDL_scancode.h>

#define INST_PER_FRAME 20

int init_screen();
void uninit_screen();
void clear_screen();
void update_graphics(u8 *screen);
void get_keys(u8 *keys);
uint8_t draw_sprite(u8 *screen, u8 *sprite, u8 x, u8 y);
int scancode_to_chip8(SDL_Scancode scancode);
uint32_t millis();

#endif
