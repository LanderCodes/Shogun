#include "ui/input.h"

static int key_code(SDL_Keycode key)
{
    switch (key) {
    case SDLK_UP: case SDLK_w: return 1;
    case SDLK_DOWN: case SDLK_s: return 2;
    case SDLK_LEFT: case SDLK_a: return 3;
    case SDLK_RIGHT: case SDLK_d: return 4;
    case SDLK_RETURN: case SDLK_KP_ENTER: case SDLK_SPACE: return 5;
    case SDLK_ESCAPE: case SDLK_BACKSPACE: return 6;
    default: return 0;
    }
}

void input_init(Input *input)
{
    input->pad = NULL;
    for (int i = 0; i < 7; ++i) {
        input->previous[i] = 0;
        input->repeat[i] = 0;
    }
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            input->pad = SDL_GameControllerOpen(i);
            break;
        }
    }
}

void input_done(Input *input)
{
    if (input->pad) SDL_GameControllerClose(input->pad);
}

void input_event(Input *input, const SDL_Event *event, Game *game, App *app)
{
    switch (event->type) {
    case SDL_QUIT:
        break;
    case SDL_TEXTINPUT:
        if (game->screen == SCREEN_ONLINE_NAME) {
            game_name_input(game, event->text.text);
        } else if (game->screen == SCREEN_ONLINE_SETUP) {
            game_setup_text(game, event->text.text);
        }
        break;
    case SDL_KEYDOWN: {
        SDL_Keycode key = event->key.keysym.sym;
        SDL_Keymod mod = event->key.keysym.mod;
        if (game->screen == SCREEN_ONLINE_NAME && key == SDLK_BACKSPACE) {
            game_name_backspace(game);
            break;
        }
        if (game->screen == SCREEN_ONLINE_SETUP && key == SDLK_BACKSPACE) {
            game_setup_backspace(game);
            break;
        }
        if (game_text_input_active(game)) {
            if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
                game_key(game, 5);
            } else if (key == SDLK_ESCAPE) {
                game_key(game, 6);
            }
            break;
        }
        if (key == SDLK_F11 || key == SDLK_f ||
            (key == SDLK_RETURN && (mod & KMOD_ALT))) {
            app_toggle_fullscreen(app);
            break;
        }
        int code = key_code(key);
        if (code && (!event->key.repeat || code <= 4)) game_key(game, code);
        break;
    }
    case SDL_MOUSEMOTION: {
        float x;
        float y;
        app_to_logical(app, event->motion.x, event->motion.y, &x, &y);
        game_mouse_move(game, x, y, app->portrait);
        if (game_hovering_button(game)) SDL_SetCursor(SDL_GetDefaultCursor());
        break;
    }
    case SDL_MOUSEBUTTONDOWN:
        if (event->button.button == SDL_BUTTON_LEFT) {
            float x;
            float y;
            app_to_logical(app, event->button.x, event->button.y, &x, &y);
            game_mouse_down(game, x, y, app->portrait);
        }
        break;
    case SDL_WINDOWEVENT:
        if (event->window.event == SDL_WINDOWEVENT_RESIZED ||
            event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
            app_layout(app);
        }
        break;
    case SDL_CONTROLLERDEVICEADDED:
        if (!input->pad && SDL_IsGameController(event->cdevice.which)) {
            input->pad = SDL_GameControllerOpen(event->cdevice.which);
        }
        break;
    case SDL_CONTROLLERDEVICEREMOVED:
        if (input->pad &&
            SDL_GameControllerGetPlayerIndex(input->pad) == event->cdevice.which) {
            SDL_GameControllerClose(input->pad);
            input->pad = NULL;
        }
        break;
    default:
        break;
    }
}

void input_poll(Input *input, Uint32 now, Game *game)
{
    if (game_text_input_active(game)) {
        for (int i = 0; i < 7; ++i) input->previous[i] = 0;
        return;
    }

    int state[7] = {0};
    if (input->pad) {
        SDL_GameController *pad = input->pad;
        if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_UP) ||
            SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTY) < -16000) {
            state[1] = 1;
        }
        if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN) ||
            SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTY) > 16000) {
            state[2] = 1;
        }
        if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_LEFT) ||
            SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTX) < -16000) {
            state[3] = 1;
        }
        if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) ||
            SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTX) > 16000) {
            state[4] = 1;
        }
        if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_A)) state[5] = 1;
        if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_B) ||
            SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_START)) {
            state[6] = 1;
        }
    }

    for (int k = 1; k <= 6; ++k) {
        if (state[k] && !input->previous[k]) {
            game_key(game, k);
            input->repeat[k] = now + 350;
        } else if (state[k] && k <= 4 && now >= input->repeat[k]) {
            game_key(game, k);
            input->repeat[k] = now + 120;
        }
        input->previous[k] = state[k];
    }
}
