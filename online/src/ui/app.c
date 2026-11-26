#include "ui/app.h"

#include "core/types.h"
#include "render/layout.h"

#include <stdio.h>

static SDL_Texture *make_target(App *app, int w, int h)
{
    if (app->target && app->target_w == w && app->target_h == h) {
        return app->target;
    }
    if (app->target) SDL_DestroyTexture(app->target);
    app->target = SDL_CreateTexture(app->renderer, SDL_PIXELFORMAT_RGBA8888,
                                    SDL_TEXTUREACCESS_TARGET, w, h);
    if (!app->target) {
        fprintf(stderr, "SDL_CreateTexture: %s\n", SDL_GetError());
        return NULL;
    }
    SDL_SetTextureBlendMode(app->target, SDL_BLENDMODE_NONE);
    app->target_w = w;
    app->target_h = h;
    return app->target;
}

int app_init(App *app, const char *title)
{
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 0;
    }

    app->window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED, LOGICAL_W, LOGICAL_H,
                                   SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
    if (!app->window) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        return 0;
    }
    app->renderer = SDL_CreateRenderer(app->window, -1,
                                       SDL_RENDERER_ACCELERATED |
                                       SDL_RENDERER_PRESENTVSYNC);
    if (!app->renderer) {
        app->renderer = SDL_CreateRenderer(app->window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!app->renderer) {
        fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
        return 0;
    }
    app->target = NULL;
    app->target_w = 0;
    app->target_h = 0;
    return 1;
}

void app_done(App *app)
{
    if (app->target) SDL_DestroyTexture(app->target);
    if (app->renderer) SDL_DestroyRenderer(app->renderer);
    if (app->window) SDL_DestroyWindow(app->window);
    SDL_Quit();
}

void app_layout(App *app)
{
    int w;
    int h;
    SDL_GetWindowSize(app->window, &w, &h);
    app->portrait = h > w;
    app->lw = layout_width(app->portrait);
    app->lh = layout_height(app->portrait);
    if (!make_target(app, app->lw, app->lh)) {
        fprintf(stderr, "could not create render target\n");
    }
}

void app_begin(App *app)
{
    SDL_SetRenderTarget(app->renderer, app->target);
}

void app_end(App *app)
{
    int w;
    int h;
    SDL_GetRendererOutputSize(app->renderer, &w, &h);
    float scale = w / (float)app->lw;
    float alt = h / (float)app->lh;
    if (alt < scale) scale = alt;

    SDL_FRect view = {((float)w - app->lw * scale) / 2.0f,
                      ((float)h - app->lh * scale) / 2.0f,
                      app->lw * scale, app->lh * scale};
    SDL_SetRenderTarget(app->renderer, NULL);
    SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 255);
    SDL_RenderClear(app->renderer);
    SDL_RenderCopyF(app->renderer, app->target, NULL, &view);
    SDL_RenderPresent(app->renderer);
}

void app_to_logical(const App *app, int px, int py, float *x, float *y)
{
    int w;
    int h;
    SDL_GetRendererOutputSize(app->renderer, &w, &h);
    float scale = w / (float)app->lw;
    float alt = h / (float)app->lh;
    if (alt < scale) scale = alt;
    float ox = ((float)w - app->lw * scale) / 2.0f;
    float oy = ((float)h - app->lh * scale) / 2.0f;
    *x = (px - ox) / scale;
    *y = (py - oy) / scale;
}

void app_toggle_fullscreen(App *app)
{
    Uint32 flags = SDL_GetWindowFlags(app->window) & SDL_WINDOW_FULLSCREEN_DESKTOP;
    SDL_SetWindowFullscreen(app->window, flags ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
    app_layout(app);
}
