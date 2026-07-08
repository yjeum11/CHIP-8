#include <SDL3/SDL_events.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_timer.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include <SDL3/SDL.h>

#include "util.h"
#include "screen.h"

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

const static u32 gamepixel_w = 10;
const static u32 gamepixel_h = 10;

const static u32 width = gamepixel_w * 64;
const static u32 height = gamepixel_h * 32;

int init_screen() {
    SDL_SetAppMetadata("Example Renderer Clear", "1.0", "com.example.renderer-clear");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return -1;
    }

    if (!SDL_CreateWindowAndRenderer("examples/renderer/clear", width, height, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return -1;
    }
    // SDL_SetRenderLogicalPresentation(renderer, width, height, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    return 0;
}

void clear_screen() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}

void update_graphics(uint8_t *screen) {
    SDL_FRect render_rects[64 * 32] = {0};
    int rect_idx = 0;
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 64; j++) {
            if (screen[i*64+j]) {
                render_rects[rect_idx].x = j * gamepixel_w;
                render_rects[rect_idx].y = i * gamepixel_h;
                render_rects[rect_idx].w = gamepixel_w;
                render_rects[rect_idx].h = gamepixel_h;
                rect_idx++;
            }
        }
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRects(renderer, render_rects, rect_idx);
    SDL_RenderPresent(renderer);
}

// u8 *keys owned by caller. keys must have >=16 elements
void get_keys(u8 *keys) {
    const bool *sdl_keys = SDL_GetKeyboardState(NULL);
    keys[1] = sdl_keys[SDL_SCANCODE_1] ? 1 : 0;
    keys[2] = sdl_keys[SDL_SCANCODE_2] ? 1 : 0;
    keys[3] = sdl_keys[SDL_SCANCODE_3] ? 1 : 0;
    keys[0xC] = sdl_keys[SDL_SCANCODE_4] ? 1 : 0;
    keys[4] = sdl_keys[SDL_SCANCODE_Q] ? 1 : 0;
    keys[5] = sdl_keys[SDL_SCANCODE_W] ? 1 : 0;
    keys[6] = sdl_keys[SDL_SCANCODE_E] ? 1 : 0;
    keys[0xD] = sdl_keys[SDL_SCANCODE_R] ? 1 : 0;
    keys[7] = sdl_keys[SDL_SCANCODE_A] ? 1 : 0;
    keys[8] = sdl_keys[SDL_SCANCODE_S] ? 1 : 0;
    keys[9] = sdl_keys[SDL_SCANCODE_D] ? 1 : 0;
    keys[0xE] = sdl_keys[SDL_SCANCODE_F] ? 1 : 0;
    keys[0xA] = sdl_keys[SDL_SCANCODE_Z] ? 1 : 0;
    keys[0] = sdl_keys[SDL_SCANCODE_X] ? 1 : 0;
    keys[0xB] = sdl_keys[SDL_SCANCODE_C] ? 1 : 0;
    keys[0xF] = sdl_keys[SDL_SCANCODE_V] ? 1 : 0;
    keys[16] = sdl_keys[SDL_SCANCODE_ESCAPE] ? 1 : 0;
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

u32 millis() {
    return SDL_GetTicks();
}

