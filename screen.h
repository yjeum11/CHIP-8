#ifndef SCREEN_H
#define SCREEN_H

#include "util.h"

#define INST_PER_FRAME 100

int init_screen();
void uninit_screen();
void clear_screen();
void update_graphics(u8 *screen);
void get_keys(u8 *keys);
uint8_t draw_sprite(u8 *screen, u8 *sprite, u8 x, u8 y);
uint8_t keyboard_to_chip8(u8 key);
uint32_t millis();

#endif
