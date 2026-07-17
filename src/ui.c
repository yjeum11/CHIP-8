#include <SDL3/SDL_render.h>
#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <string.h>
#include "microui.h"
#include "ui.h"

static int text_height(mu_Font font) {
    return 8;
}

static int text_width(mu_Font font, const char *str, int len) {
    if (len == -1)
        len = strlen(str);
    return 8 * len;
}

void init_ui(mu_Context *ctx) {
    mu_init(ctx);
    ctx->text_height = text_height;
    ctx->text_width = text_width;
}

int handle_events_ui(mu_Context *ctx, SDL_Event *event) {
    int ret = 0;
    switch (event->type) {
        case SDL_EVENT_MOUSE_MOTION:
            mu_input_mousemove(ctx, event->motion.x, event->motion.y);
            ret = 1;
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            int b = button_map[event->button.button & 0xff];
          if (b && event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) { mu_input_mousedown(ctx, event->button.x, event->button.y, b); }
          if (b && event->type ==   SDL_EVENT_MOUSE_BUTTON_UP) { mu_input_mouseup(ctx, event->button.x, event->button.y, b);   }
            ret = 1;
          break;
        }
    }
    return ret;
}

void test_window(mu_Context *ctx) {
    if (mu_begin_window(ctx, "My Window", mu_rect(10, 10, 140, 86))) {
        mu_layout_row(ctx, 2, (int[]) { 60, -1 }, 0);

        mu_label(ctx, "First:");
        if (mu_button(ctx, "Button1")) {
            printf("Button1 pressed\n");
        }

        mu_label(ctx, "Second:");
        if (mu_button(ctx, "Button2")) {
            mu_open_popup(ctx, "My Popup");
        }

        if (mu_begin_popup(ctx, "My Popup")) {
            mu_label(ctx, "Hello world!");
            mu_end_popup(ctx);
        }

        mu_end_window(ctx);
    }
}

void process_ui_frame(mu_Context *ctx) {
    mu_begin(ctx);
    test_window(ctx);
    mu_end(ctx);
}

void render_text(SDL_Renderer *renderer, const char *str, mu_Vec2 pos, mu_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDebugText(renderer, pos.x, pos.y, str);
}

void render_rect(SDL_Renderer *renderer, mu_Rect rect, mu_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_FRect sdl_rect = {.x = (float)rect.x, .y = (float)rect.y, .w = (float)rect.w, .h = (float)rect.h};
    SDL_RenderFillRect(renderer, &sdl_rect);
}

void render_clip(SDL_Renderer *renderer, mu_Rect rect) {
    SDL_Rect sdl_rect = {.x = rect.x, .y = rect.y, .w = rect.w, .h = rect.h};
    SDL_SetRenderClipRect(renderer, &sdl_rect);
}

void render_ui(mu_Context *ctx, SDL_Renderer *renderer) {
    mu_Command *cmd = NULL;
    while (mu_next_command(ctx, &cmd)) {
        switch (cmd->type) {
            case MU_COMMAND_TEXT: render_text(renderer, cmd->text.str, cmd->text.pos, cmd->text.color); break;
            case MU_COMMAND_RECT: render_rect(renderer, cmd->rect.rect, cmd->rect.color); break;
            case MU_COMMAND_CLIP: render_clip(renderer, cmd->rect.rect); break;
        }
    }
}
