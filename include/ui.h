#ifndef UI_H
#define UI_H

#include <SDL3/SDL.h>
#include "microui.h"

#include "chip8.h"

static const char button_map[256] = {
  [ SDL_BUTTON_LEFT   & 0xff ] =  MU_MOUSE_LEFT,
  [ SDL_BUTTON_RIGHT  & 0xff ] =  MU_MOUSE_RIGHT,
  [ SDL_BUTTON_MIDDLE & 0xff ] =  MU_MOUSE_MIDDLE,
};


void init_ui(mu_Context *ctx);

int handle_events_ui(mu_Context *ctx, SDL_Event *event);

void process_ui_frame(mu_Context *ctx, Chip8 *chip8, Chip8_State *state);

void render_ui(mu_Context *ctx, SDL_Renderer *renderer);

#endif
