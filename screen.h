#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>

void init_screen();
void uninit_screen();
void clear_screen();
void update_graphics(uint8_t *screen);
void draw();
int16_t get_key();
int16_t get_key_block();
uint8_t draw_sprite(uint8_t *screen, uint8_t *sprite, uint8_t x, uint8_t y);
uint8_t keyboard_to_chip8(uint8_t key);
uint32_t millis(struct timespec *ts);

#endif
