#ifndef SHOGUN_INPUT_H
#define SHOGUN_INPUT_H

#include <SDL2/SDL.h>

#include "app.h"
#include "game.h"

typedef struct {
    SDL_GameController *pad;
    int previous[7];
    Uint32 repeat[7];
} Input;

void input_init(Input *input);
void input_done(Input *input);
void input_event(Input *input, const SDL_Event *event, Game *game, App *app);
void input_poll(Input *input, Uint32 now, Game *game);

#endif
