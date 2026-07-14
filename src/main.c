#include <SDL3/SDL_events.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "util.h"
#include "screen.h"
#include "chip8.h"

static Chip8 *chip8;
static Chip8_State state;
static u32 timestamp;
static u8 keys[17];

SDL_AppResult SDL_AppInit (void **appstate, int argc, char *argv[]) {
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

    // state consists of waiting and redraw
    // redraw is a pure output. need to or across iterations
    // waiting we need to pass through every iteration

    for (int i = 0; i < INST_PER_FRAME; i++) {
        Chip8_State next_state = chip8_execute(chip8, keys, state);
        state.redraw |= next_state.redraw;
        state.waiting_key = next_state.waiting_key;
        state.waiting = next_state.waiting;
        if (state.waiting) {
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

    if (state.redraw)
        update_graphics(chip8->display);
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    if (event->type == SDL_EVENT_KEY_UP && state.waiting) {
        int key_released = scancode_to_chip8(event->key.scancode);
        if (key_released != -1) {
            state.waiting = 0;
            state.waiting_key = key_released;
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

