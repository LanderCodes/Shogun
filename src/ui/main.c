#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdio.h>

#include "ui/app.h"
#include "ui/input.h"

#include "core/config.h"
#include "core/game.h"
#include "render/art.h"
#include "render/render.h"

#define STEP_MS 33

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    App app;
    if (!app_init(&app, "SHOGUN - TCKSoft (pure C)")) return 1;
    app_layout(&app);

    Art art;
    art_init(&art, app.renderer);

    Input input;
    input_init(&input);

    Settings settings;
    settings_init(&settings);

    uint32_t seed = SDL_GetTicks() ^ (uint32_t)(uintptr_t)&app;
    Game game;
    game_init(&game, seed, &settings);

    int running = 1;
    Uint32 last = SDL_GetTicks();
    Uint32 accumulator = 0;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = 0;
            input_event(&input, &event, &game, &app);
        }
        if (!running) break;

        Uint32 now = SDL_GetTicks();
        accumulator += now - last;
        last = now;
        if (accumulator > 250) accumulator = 250;
        while (accumulator >= STEP_MS) {
            game_update(&game);
            accumulator -= STEP_MS;
        }

        input_poll(&input, now, &game);
        app_begin(&app);
        render_screen(&art, &game, app.portrait, now);
        app_end(&app);
    }

    input_done(&input);
    art_done(&art);
    app_done(&app);
    return 0;
}
