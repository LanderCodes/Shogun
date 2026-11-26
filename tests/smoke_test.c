#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>

#include "core/config.h"
#include "core/game.h"
#include "render/art.h"
#include "render/render.h"

static SDL_Renderer *renderer;
static SDL_PixelFormat *pixel_format;

static void fail(const char *message)
{
    fprintf(stderr, "smoke test: %s (%s)\n", message, SDL_GetError());
    exit(1);
}

static unsigned int pixel(int x, int y)
{
    Uint32 raw;
    SDL_Rect rect = {x, y, 1, 1};
    if (SDL_RenderReadPixels(renderer, &rect, SDL_PIXELFORMAT_RGBA8888, &raw, 4)) {
        fail("SDL_RenderReadPixels");
    }
    return (unsigned int)raw;
}

static void rgb(unsigned int px, int *r, int *g, int *b)
{
    Uint8 rr, gg, bb;
    SDL_GetRGB(px, pixel_format, &rr, &gg, &bb);
    *r = (int)rr;
    *g = (int)gg;
    *b = (int)bb;
}

static void check_blue(int x, int y)
{
    int r, g, b;
    rgb(pixel(x, y), &r, &g, &b);
    if (r != 0 || g != 0 || b != 64) {
        fprintf(stderr, "smoke test: expected blue at (%d,%d), got %d %d %d\n",
                x, y, r, g, b);
        exit(1);
    }
}

static void set_target(SDL_Texture *target)
{
    if (SDL_SetRenderTarget(renderer, target)) fail("SDL_SetRenderTarget");
}

int main(void)
{
    if (SDL_Init(SDL_INIT_VIDEO)) fail("SDL_Init");
    pixel_format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888);
    if (!pixel_format) fail("SDL_AllocFormat");
    SDL_Window *window = SDL_CreateWindow("smoke", SDL_WINDOWPOS_CENTERED,
                                         SDL_WINDOWPOS_CENTERED, 960, 540,
                                         SDL_WINDOW_HIDDEN);
    if (!window) fail("SDL_CreateWindow");
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) fail("SDL_CreateRenderer");

    SDL_Texture *landscape = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                               SDL_TEXTUREACCESS_TARGET, 960, 540);
    SDL_Texture *portrait = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                              SDL_TEXTUREACCESS_TARGET, 480, 800);
    if (!landscape || !portrait) fail("SDL_CreateTexture");

    Art art;
    art_init(&art, renderer);
    Settings settings;
    settings_init(&settings);
    Game game;
    game_init(&game, 12345u, &settings);

    set_target(landscape);
    render_screen(&art, &game, 0, 0);
    unsigned int light = pixel(10, 10);
    unsigned int dark = pixel(70, 10);
    if (light == dark) {
        fprintf(stderr, "smoke test: menu checkerboard is missing\n");
        return 1;
    }

    game.two_player = 1;
    game.screen = SCREEN_PLAY;
    render_screen(&art, &game, 0, 0);
    check_blue(520, 100);
    check_blue(735, 300);
    check_blue(900, 520);

    game.screen = SCREEN_PAUSE;
    render_screen(&art, &game, 0, 0);

    game.screen = SCREEN_WINNER;
    game.winner = SIDE_WHITE;
    game.win_timer = 30;
    render_screen(&art, &game, 0, 0);
    check_blue(520, 100);
    game_mouse_down(&game, 735.0f, 325.0f, 0);
    if (game.screen != SCREEN_PLAY || game.winner != -1) {
        fprintf(stderr, "smoke test: rematch button failed\n");
        return 1;
    }
    game.screen = SCREEN_WINNER;
    game.winner = SIDE_RED;
    game.winner_cursor = 1;
    game_key(&game, 5);
    if (game.screen != SCREEN_MENU) {
        fprintf(stderr, "smoke test: winner main menu button failed\n");
        return 1;
    }

    set_target(portrait);
    game.screen = SCREEN_MENU;
    render_screen(&art, &game, 1, 0);
    game.two_player = 1;
    game.screen = SCREEN_PLAY;
    render_screen(&art, &game, 1, 0);
    check_blue(460, 700);

    art_done(&art);
    SDL_DestroyTexture(portrait);
    SDL_DestroyTexture(landscape);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_FreeFormat(pixel_format);
    SDL_Quit();
    puts("smoke test: ok");
    return 0;
}
