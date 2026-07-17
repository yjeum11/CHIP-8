#include <stdint.h>
#include <stdio.h>

#include <SDL3/SDL.h>
#include "microui.h"

#include "ui.h"
#include "util.h"
#include "screen.h"

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_AudioStream *audiostream = NULL;
static int sample_num = 0;

static const u32 gamepixel_w = 10;
static const u32 gamepixel_h = 10;

static const u32 width = gamepixel_w * 64;
static const u32 height = gamepixel_h * 32;

static const SDL_AudioSpec spec = {
    .format = SDL_AUDIO_F32LE,
    .channels = 1,
    .freq = 44100,
};

int init_screen() {
    SDL_SetAppMetadata("CHIP-8", "1.0", "com.yjeum.chip8");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return -1;
    }

    if (!SDL_CreateWindowAndRenderer("CHIP-8", width, height, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return -1;
    }

    audiostream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (audiostream == NULL) {
        printf("Couldn't create audio device: %s", SDL_GetError());
    }

    SDL_ResumeAudioStreamDevice(audiostream);
    SDL_SetRenderLogicalPresentation(renderer, width, height, SDL_LOGICAL_PRESENTATION_LETTERBOX);
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
}

void render_present() {
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

int scancode_to_chip8(SDL_Scancode scancode) {
    if (scancode == SDL_SCANCODE_1) return 1;
    if (scancode == SDL_SCANCODE_2) return 2;
    if (scancode == SDL_SCANCODE_3) return 3;
    if (scancode == SDL_SCANCODE_4) return 0xC;
    if (scancode == SDL_SCANCODE_Q) return 4;
    if (scancode == SDL_SCANCODE_W) return 5;
    if (scancode == SDL_SCANCODE_E) return 6;
    if (scancode == SDL_SCANCODE_R) return 0xD;
    if (scancode == SDL_SCANCODE_A) return 7;
    if (scancode == SDL_SCANCODE_S) return 8;
    if (scancode == SDL_SCANCODE_D) return 9;
    if (scancode == SDL_SCANCODE_F) return 0xE;
    if (scancode == SDL_SCANCODE_Z) return 0xA;
    if (scancode == SDL_SCANCODE_X) return 0;
    if (scancode == SDL_SCANCODE_C) return 0xB;
    if (scancode == SDL_SCANCODE_V) return 0xF;
    return -1;
}

void uninit_screen() {
}

void play_tone() {
    const float tone_freq = 440;
    if (SDL_AudioStreamDevicePaused(audiostream)) {
        SDL_ResumeAudioStreamDevice(audiostream);
    }
    if (SDL_GetAudioStreamQueued(audiostream) < 4096 * sizeof(float)) {
        float samples[4096];
        for (u32 i = 0; i < SDL_arraysize(samples); i++) {
            samples[i] = 0.2 * SDL_sinf(sample_num * (tone_freq / 44100.f) * 2.0f * SDL_PI_F);
            sample_num++;
        }
        sample_num %= 44100;

        SDL_PutAudioStreamData(audiostream, samples, sizeof(samples));
    }
}

void pause_tone() {
    if (!SDL_AudioStreamDevicePaused(audiostream)) {
        SDL_PauseAudioStreamDevice(audiostream);
    }
}

u32 millis() {
    return SDL_GetTicks();
}

void draw_ui(mu_Context *ctx) {
    render_ui(ctx, renderer);
}
