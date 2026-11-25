#include <SDL2/SDL.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "game.h"
#include "assets.h"

#define MAX_IMAGES 8
#define MAX_TINTS 256
#define UPDATE_HZ 30

struct w2c_env {
    int unused;
};

typedef struct {
    SDL_Texture *tex;
    int id;
    uint8_t r;
    uint8_t g;
    uint8_t b;
} TintEntry;

typedef struct {
    SDL_Texture *tex;
    int32_t w;
    int32_t h;
} RenderTarget;

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *base[MAX_IMAGES];
    TintEntry tints[MAX_TINTS];
    int tint_count;
    RenderTarget target;
    int32_t lw;
    int32_t lh;
    uint8_t cr;
    uint8_t cg;
    uint8_t cb;
    float alpha;
} App;

static App app;
static w2c_shogun game;
static struct w2c_env game_env;
static int critical_hit_enabled;
static Uint32 critical_hit;
static Uint32 critical_deaths;
static Uint32 critical_last;

static void mkdirs(const char *path)
{
    char buf[1024];
    size_t n = strlen(path);
    if (n >= sizeof(buf)) {
        return;
    }
    memcpy(buf, path, n + 1);
    for (char *p = buf + 1; *p; ++p) {
        if (*p == '/') {
            *p = '\0';
            mkdir(buf, 0700);
            *p = '/';
        }
    }
    mkdir(buf, 0700);
}

static void config_file(char *out, size_t size)
{
    const char *xdg = getenv("XDG_CONFIG_HOME");
    if (xdg && *xdg) {
        snprintf(out, size, "%s/tcksoft/shogun.cfg", xdg);
        return;
    }
    const char *home = getenv("HOME");
    if (!home || !*home) {
        home = ".";
    }
    snprintf(out, size, "%s/.config/tcksoft/shogun.cfg", home);
}

static int32_t load_settings(void)
{
    char path[1024];
    config_file(path, sizeof(path));
    FILE *f = fopen(path, "r");
    if (!f) {
        return -1;
    }
    int32_t value = -1;
    if (fscanf(f, "%d", &value) != 1) {
        value = -1;
    }
    fclose(f);
    return value;
}

static void save_settings(int32_t value)
{
    char path[1024];
    char dir[1024];
    config_file(path, sizeof(path));
    snprintf(dir, sizeof(dir), "%s", path);
    char *slash = strrchr(dir, '/');
    if (!slash) {
        return;
    }
    *slash = '\0';
    mkdirs(dir);
    FILE *f = fopen(path, "w");
    if (!f) {
        return;
    }
    fprintf(f, "%d\n", value);
    fclose(f);
}

static SDL_Texture *base_texture(int id)
{
    if (id < 0 || id >= MAX_IMAGES || id >= (int)g_image_count) {
        return NULL;
    }
    if (app.base[id]) {
        return app.base[id];
    }
    const ShogunImage *img = &g_images[id];
    SDL_Texture *tex = SDL_CreateTexture(app.renderer, SDL_PIXELFORMAT_RGBA8888,
                                         SDL_TEXTUREACCESS_STATIC, img->w, img->h);
    if (!tex) {
        return NULL;
    }
    SDL_UpdateTexture(tex, NULL, img->data, img->w * 4);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    app.base[id] = tex;
    return tex;
}

static SDL_Texture *tinted_texture(int id)
{
    if (app.cr == 255 && app.cg == 255 && app.cb == 255) {
        return base_texture(id);
    }
    for (int i = 0; i < app.tint_count; ++i) {
        TintEntry *e = &app.tints[i];
        if (e->id == id && e->r == app.cr && e->g == app.cg && e->b == app.cb && e->tex) {
            return e->tex;
        }
    }
    const ShogunImage *img = &g_images[id];
    size_t count = (size_t)img->w * img->h * 4;
    uint8_t *pixels = malloc(count);
    if (!pixels) {
        return NULL;
    }
    memcpy(pixels, img->data, count);
    for (size_t i = 0; i < count; i += 4) {
        pixels[i] = (uint8_t)((uint32_t)pixels[i] * app.cr / 255);
        pixels[i + 1] = (uint8_t)((uint32_t)pixels[i + 1] * app.cg / 255);
        pixels[i + 2] = (uint8_t)((uint32_t)pixels[i + 2] * app.cb / 255);
    }
    SDL_Texture *tex = SDL_CreateTexture(app.renderer, SDL_PIXELFORMAT_RGBA8888,
                                         SDL_TEXTUREACCESS_STATIC, img->w, img->h);
    if (tex) {
        SDL_UpdateTexture(tex, NULL, pixels, img->w * 4);
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    }
    free(pixels);
    if (!tex) {
        return NULL;
    }
    TintEntry *e;
    if (app.tint_count < MAX_TINTS) {
        e = &app.tints[app.tint_count++];
    } else {
        e = &app.tints[0];
        if (e->tex) {
            SDL_DestroyTexture(e->tex);
        }
    }
    e->tex = tex;
    e->id = id;
    e->r = app.cr;
    e->g = app.cg;
    e->b = app.cb;
    return tex;
}

void w2c_env_P_sound(struct w2c_env *env, u32 id, u32 channel)
{
    (void)env;
    (void)id;
    (void)channel;
}

void w2c_env_P_alpha(struct w2c_env *env, f32 value)
{
    (void)env;
    app.alpha = value;
    if (app.alpha < 0.0f) {
        app.alpha = 0.0f;
    } else if (app.alpha > 1.0f) {
        app.alpha = 1.0f;
    }
}

void w2c_env_P_color(struct w2c_env *env, u32 r, u32 g, u32 b)
{
    (void)env;
    app.cr = (uint8_t)r;
    app.cg = (uint8_t)g;
    app.cb = (uint8_t)b;
}

void w2c_env_P_rect(struct w2c_env *env, f32 x, f32 y, f32 w, f32 h)
{
    (void)env;
    if ((int)roundf(y) == app.lh - 8) {
        return;
    }
    Uint8 a = (Uint8)(app.alpha * 255.0f);
    SDL_SetRenderDrawColor(app.renderer, app.cr, app.cg, app.cb, a);
    SDL_Rect rect;
    rect.x = (int)roundf(x);
    rect.y = (int)roundf(y);
    rect.w = (int)roundf(w);
    rect.h = (int)roundf(h);
    SDL_RenderFillRect(app.renderer, &rect);
}

void w2c_env_P_cls(struct w2c_env *env, u32 r, u32 g, u32 b)
{
    (void)env;
    SDL_SetRenderDrawColor(app.renderer, (Uint8)r, (Uint8)g, (Uint8)b, 255);
    SDL_RenderClear(app.renderer);
}

static void draw_frame(int32_t iid, u32 frame, f32 x, f32 y, f32 rot,
                       f32 sx, f32 sy)
{
    if (iid < 0 || iid >= MAX_IMAGES || iid >= (int)g_image_count) {
        return;
    }
    SDL_Texture *tex = tinted_texture(iid);
    if (!tex) {
        return;
    }
    const ShogunImage *img = &g_images[iid];
    int cols = img->frame_w ? img->w / img->frame_w : 1;
    int rows = img->frame_h ? img->h / img->frame_h : 1;
    if (cols < 1) {
        cols = 1;
    }
    if (rows < 1) {
        rows = 1;
    }
    int total = cols * rows;
    int f = (int32_t)frame % total;
    if (f < 0) {
        f += total;
    }
    SDL_Rect src;
    src.x = (f % cols) * img->frame_w;
    src.y = (f / cols) * img->frame_h;
    src.w = img->frame_w;
    src.h = img->frame_h;
    float fw = (float)img->frame_w * sx;
    float fh = (float)img->frame_h * sy;
    SDL_FRect dst;
    dst.x = x - fw / 2.0f;
    dst.y = y - fh / 2.0f;
    dst.w = fw;
    dst.h = fh;
    SDL_SetTextureAlphaMod(tex, (Uint8)(app.alpha * 255.0f));
    SDL_RenderCopyExF(app.renderer, tex, &src, &dst, rot, NULL, SDL_FLIP_NONE);
}

void w2c_env_P_img(struct w2c_env *env, u32 id, u32 frame, f32 x, f32 y,
                   f32 rot, f32 sx, f32 sy)
{
    (void)env;
    if (id == 5u && frame == 2u) {
        return;
    }
    draw_frame((int32_t)id, frame, x, y, rot, sx, sy);
}

void w2c_env_P_save(struct w2c_env *env, u32 value)
{
    (void)env;
    save_settings((int32_t)value);
}

static void ensure_target(int32_t w, int32_t h)
{
    if (app.target.tex && app.target.w == w && app.target.h == h) {
        return;
    }
    if (app.target.tex) {
        SDL_DestroyTexture(app.target.tex);
    }
    app.target.tex = SDL_CreateTexture(app.renderer, SDL_PIXELFORMAT_RGBA8888,
                                       SDL_TEXTUREACCESS_TARGET, w, h);
    if (!app.target.tex) {
        fprintf(stderr, "could not create render target: %s\n", SDL_GetError());
        exit(1);
    }
    SDL_SetTextureBlendMode(app.target.tex, SDL_BLENDMODE_NONE);
    app.target.w = w;
    app.target.h = h;
}

static void layout(void)
{
    int w;
    int h;
    SDL_GetWindowSize(app.window, &w, &h);
    w2c_shogun_G_set_portrait(&game, h > w ? 1u : 0u);
    app.lw = (int32_t)w2c_shogun_G_width(&game);
    app.lh = (int32_t)w2c_shogun_G_height(&game);
    if (app.lw <= 0 || app.lh <= 0) {
        fprintf(stderr, "invalid logical size\n");
        exit(1);
    }
    ensure_target(app.lw, app.lh);
}

static void view_rect(SDL_FRect *out)
{
    int w;
    int h;
    SDL_GetRendererOutputSize(app.renderer, &w, &h);
    float scale = fminf((float)w / app.lw, (float)h / app.lh);
    out->w = app.lw * scale;
    out->h = app.lh * scale;
    out->x = ((float)w - out->w) / 2.0f;
    out->y = ((float)h - out->h) / 2.0f;
}

static void to_logical(int px, int py, float *x, float *y)
{
    SDL_FRect rect;
    view_rect(&rect);
    *x = ((float)px - rect.x) / (rect.w / app.lw);
    *y = ((float)py - rect.y) / (rect.h / app.lh);
}

static void render_frame(Uint32 now)
{
    SDL_FRect rect;
    view_rect(&rect);
    SDL_SetRenderDrawColor(app.renderer, 0, 0, 0, 255);
    SDL_RenderClear(app.renderer);
    SDL_SetRenderTarget(app.renderer, app.target.tex);
    int game_state = (int)game.w2c_memory.data[5456];
    int playing = game_state == 1;
    if (critical_hit_enabled && playing && now - critical_last > 5000) {
        critical_last = now;
        if (!(rand() % 2)) {
            ++critical_deaths;
        }
    }
    critical_hit = playing && critical_deaths > 0 ? 1u : 0u;
    w2c_shogun_G_render(&game, (u32)now);
    if (critical_hit > 0 && rand() % 8 == 0) {
        w2c_env_P_color(&game_env, 255u, 0u, 0u);
        w2c_env_P_rect(&game_env, 0.0f, 0.0f, (f32)app.lw, (f32)app.lh);
    }
    SDL_SetRenderTarget(app.renderer, NULL);
    SDL_RenderCopyF(app.renderer, app.target.tex, NULL, &rect);
    SDL_RenderPresent(app.renderer);
}

static int key_code(SDL_Keycode key)
{
    switch (key) {
    case SDLK_UP: return 1;
    case SDLK_DOWN: return 2;
    case SDLK_LEFT: return 3;
    case SDLK_RIGHT: return 4;
    case SDLK_RETURN: return 5;
    case SDLK_KP_ENTER: return 5;
    case SDLK_SPACE: return 5;
    case SDLK_ESCAPE: return 6;
    case SDLK_BACKSPACE: return 6;
    case SDLK_w: return 1;
    case SDLK_s: return 2;
    case SDLK_a: return 3;
    case SDLK_d: return 4;
    default: return 0;
    }
}

static void toggle_fullscreen(void)
{
    Uint32 flags = SDL_GetWindowFlags(app.window) & SDL_WINDOW_FULLSCREEN_DESKTOP;
    SDL_SetWindowFullscreen(app.window, flags ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
    layout();
}

static SDL_GameController *pad;
static int pad_prev[7];
static Uint32 pad_repeat[7];

static void poll_pad(Uint32 now)
{
    int state[7] = {0};
    if (pad) {
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
        if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_A)) {
            state[5] = 1;
        }
        if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_B) ||
            SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_START)) {
            state[6] = 1;
        }
    }
    for (int k = 1; k <= 6; ++k) {
        if (state[k] && !pad_prev[k]) {
            w2c_shogun_G_key(&game, (u32)k);
            pad_repeat[k] = now + 350;
        } else if (state[k] && k <= 4 && now >= pad_repeat[k]) {
            w2c_shogun_G_key(&game, (u32)k);
            pad_repeat[k] = now + 120;
        }
        pad_prev[k] = state[k];
    }
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    app.window = SDL_CreateWindow("SHOGUN - TCKSoft (no sound, full C)", SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED, 960, 540,
                                  SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
    if (!app.window) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        return 1;
    }
    app.renderer = SDL_CreateRenderer(app.window, -1,
                                      SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!app.renderer) {
        app.renderer = SDL_CreateRenderer(app.window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!app.renderer) {
        fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetRenderDrawColor(app.renderer, 0, 0, 0, 255);

    wasm_rt_init();
    wasm2c_shogun_instantiate(&game, &game_env);

    uint32_t seed = SDL_GetTicks() ^ (uint32_t)(uintptr_t)&app;
    if (getenv("SHOGUN_DEBUG")) {
        printf("seed %u\n", seed);
    }
    critical_hit_enabled = getenv("SHOGUN_CRITICAL_HIT") != NULL;
    critical_last = SDL_GetTicks();
    u32 autoplay = getenv("SHOGUN_AUTOPLAY") ? 1u : 0u;
    w2c_shogun_G_init(&game, seed, (u32)load_settings(), autoplay);
    layout();

    int running = 1;
    Uint32 last = SDL_GetTicks();
    Uint32 accumulator = 0;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                running = 0;
                break;
            case SDL_KEYDOWN: {
                SDL_Keycode key = event.key.keysym.sym;
                SDL_Keymod mod = event.key.keysym.mod;
                if (key == SDLK_F11 || (key == SDLK_RETURN && (mod & KMOD_ALT)) ||
                    key == SDLK_f) {
                    toggle_fullscreen();
                    break;
                }
                int code = key_code(key);
                if (code && (!event.key.repeat || code <= 4)) {
                    w2c_shogun_G_key(&game, (u32)code);
                }
                break;
            }
            case SDL_MOUSEMOTION: {
                float x;
                float y;
                to_logical(event.motion.x, event.motion.y, &x, &y);
                w2c_shogun_G_mousemove(&game, (f32)x, (f32)y);
                if (w2c_shogun_G_hover_button(&game)) {
                    SDL_SetCursor(SDL_GetDefaultCursor());
                }
                break;
            }
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    float x;
                    float y;
                    to_logical(event.button.x, event.button.y, &x, &y);
                    w2c_shogun_G_mousedown(&game, (f32)x, (f32)y);
                }
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED ||
                    event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    layout();
                }
                break;
            case SDL_CONTROLLERDEVICEADDED:
                if (!pad) {
                    pad = SDL_GameControllerOpen(event.cdevice.which);
                }
                break;
            case SDL_CONTROLLERDEVICEREMOVED:
                if (pad && SDL_GameControllerGetPlayerIndex(pad) == event.cdevice.which) {
                    SDL_GameControllerClose(pad);
                    pad = NULL;
                }
                break;
            default:
                break;
            }
        }

        Uint32 now = SDL_GetTicks();
        accumulator += now - last;
        last = now;
        if (accumulator > 250) {
            accumulator = 250;
        }
        while (accumulator >= 1000 / UPDATE_HZ) {
            w2c_shogun_G_update(&game);
            accumulator -= 1000 / UPDATE_HZ;
        }
        poll_pad(now);
        render_frame(now);
    }

    for (int i = 0; i < MAX_IMAGES; ++i) {
        if (app.base[i]) {
            SDL_DestroyTexture(app.base[i]);
        }
    }
    for (int i = 0; i < app.tint_count; ++i) {
        if (app.tints[i].tex) {
            SDL_DestroyTexture(app.tints[i].tex);
        }
    }
    if (app.target.tex) {
        SDL_DestroyTexture(app.target.tex);
    }
    if (pad) {
        SDL_GameControllerClose(pad);
    }
    wasm2c_shogun_free(&game);
    wasm_rt_free();
    SDL_DestroyRenderer(app.renderer);
    SDL_DestroyWindow(app.window);
    SDL_Quit();
    return 0;
}
