#include <stdio.h>
#include <SDL3/SDL.h>
#include <emscripten.h>

/* We will use this renderer to draw into this window every frame. */
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

static void mainloop() {
    const double now = ((double)SDL_GetTicks()) / 1000.0;  /* convert from milliseconds to seconds. */
    /* choose the color for the frame we will draw. The sine wave trick makes it fade between colors smoothly. */
    const float red = (float) (0.5 + 0.5 * SDL_sin(now));
    const float green = (float) (0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 2 / 3));
    const float blue = (float) (0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 4 / 3));
    SDL_SetRenderDrawColorFloat(renderer, red, green, blue, SDL_ALPHA_OPAQUE_FLOAT);  /* new color, full alpha. */

    /* clear the window to the draw color. */
    SDL_RenderClear(renderer);

    /* put the newly-cleared rendering on the screen. */
    SDL_RenderPresent(renderer);
}

int main() {
    printf("hello\n");
    SDL_Init(SDL_INIT_VIDEO);
    printf("hello\n");
    SDL_CreateWindowAndRenderer("examples/renderer/clear", 640, 480, 0, &window, &renderer);
    printf("hello\n");
    SDL_SetRenderLogicalPresentation(renderer, 640, 480, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    printf("hello\n");
    SDL_SetRenderVSync(renderer, 1);
    printf("hello\n");

    emscripten_set_main_loop(mainloop, 0, 1);
    return 0;
}

