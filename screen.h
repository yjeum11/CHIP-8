#ifndef SCREEN_H
#define SCREEN_H

#include "util.h"

void init_screen();
void uninit_screen();
void clear_screen();
void update_graphics(u8 *screen);
void get_input(u8 *inputs);
void draw();
int16_t get_key();
int16_t get_key_block();
uint8_t draw_sprite(u8 *screen, u8 *sprite, u8 x, u8 y);
uint8_t keyboard_to_chip8(u8 key);
uint32_t millis(struct timespec *ts);

#endif
