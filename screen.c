#include <curses.h>
#include <locale.h>
#include <stdint.h>
#include <time.h>



void init_screen() {
    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
}

void clear_screen() {
    clear();
}

void update_graphics(uint8_t *screen) {
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 64; j++) {
            if (screen[i*64+j]) {
                mvaddch(i, j, ACS_BLOCK);
            }
        }
    }
    refresh();
}

void draw() {
    refresh();
}

int16_t get_key() {
    return getch();
}

int16_t get_key_block() {
    nodelay(stdscr, FALSE);
    int ch = getch();
    nodelay(stdscr, TRUE);
    return ch;
}

uint8_t keyboard_to_chip8(uint8_t key) {
    switch (key) {
        case '1':
        case '2':
        case '3':
            return key - '1' + 1;
        case '4':
            return 0xC;
        case 'q':
            return 4;
        case 'w':
            return 5;
        case 'e':
            return 6;
        case 'r':
            return 0xD;
        case 'a':
            return 7;
        case 's':
            return 8;
        case 'd':
            return 9;
        case 'f':
            return 0xE;
        case 'z':
            return 0xA;
        case 'x':
            return 0;
        case 'c':
            return 0xC;
        case 'v':
            return 0xF;
        default:
            return key;
    }
}

void uninit_screen() {
    endwin();
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

uint32_t millis(struct timespec *ts) {
    timespec_get(ts, TIME_UTC);
    return ts->tv_sec * 1000 + (ts->tv_nsec / 1000000);
}
