#include "client.h"

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
    online_prefs_load(game.token, sizeof(game.token));
    snprintf(game.server, sizeof(game.server), "%s", online_default_server());

    OnlineNet net;
    online_net_init(&net);
    int text_input = 0;

    int running = 1;
    Uint32 last = SDL_GetTicks();
    Uint32 accumulator = 0;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
                game.pending_leave = 1;
            }
            input_event(&input, &event, &game, &app);
        }
        if (!running) break;

        int typing = game.screen == SCREEN_ONLINE_NAME ||
                     (game.screen == SCREEN_ONLINE_SETUP && game.setup_typing);
        if (typing && !text_input) {
            SDL_StartTextInput();
            text_input = 1;
        } else if (!typing && text_input) {
            SDL_StopTextInput();
            text_input = 0;
        }

        if (game.pending_leave) {
            online_net_send_leave(&net, game.match_id);
            online_net_close(&net);
            game.pending_leave = 0;
        }

        if (game.pending_connect) {
            game.pending_connect = 0;
            online_net_close(&net);
            if (!online_net_connect_string(&net, online_default_server(),
                                           game.name, game.token)) {
                snprintf(game.online_message, sizeof(game.online_message),
                         game.token[0] ? "COULD NOT CONNECT TO SERVER"
                                       : "ACCESS TOKEN IS NOT SET");
                game.online_phase = ONLINE_NONE;
            } else {
                snprintf(game.server, sizeof(game.server), "%s", net.server);
                online_prefs_save(game.token);
            }
        }

        if (game.pending_accept) {
            game.pending_accept = 0;
            online_net_send_accept(&net, game.match_id);
            snprintf(game.online_message, sizeof(game.online_message),
                     "WAITING FOR OPPONENT...");
        }

        if (game.pending_decline) {
            game.pending_decline = 0;
            online_net_send_decline(&net, game.match_id);
            game.screen = SCREEN_ONLINE_WAIT;
            game.online_phase = ONLINE_WAITING;
            snprintf(game.online_message, sizeof(game.online_message),
                     "CANCELLING MATCH...");
        }

        if (game.pending_rematch) {
            game.pending_rematch = 0;
            online_net_send_rematch(&net, game.match_id, 1);
        }

        online_net_update(&net, &game);

        Uint32 now = SDL_GetTicks();
        accumulator += now - last;
        last = now;
        if (accumulator > 250) accumulator = 250;
        while (accumulator >= STEP_MS) {
            game_update(&game);
            accumulator -= STEP_MS;
        }

        input_poll(&input, now, &game);

        if (game.has_pending_move && game.local_side >= 0 &&
            online_net_connected(&net)) {
            online_net_send_move(&net, game.match_id, &game.pending_move);
            game.has_pending_move = 0;
        }

        app_begin(&app);
        render_screen(&art, &game, app.portrait, now);
        app_end(&app);
    }

    online_net_send_leave(&net, game.match_id);
    online_net_close(&net);
    if (text_input) SDL_StopTextInput();
    input_done(&input);
    art_done(&art);
    app_done(&app);
    return 0;
}
