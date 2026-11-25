#ifndef SHOGUN_APP_H
#define SHOGUN_APP_H

#include <SDL2/SDL.h>

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *target;
    int target_w;
    int target_h;
    int lw;
    int lh;
    int portrait;
} App;

int app_init(App *app, const char *title);
void app_done(App *app);
void app_layout(App *app);
void app_begin(App *app);
void app_end(App *app);
void app_to_logical(const App *app, int px, int py, float *x, float *y);
void app_toggle_fullscreen(App *app);

#endif
