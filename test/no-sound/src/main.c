#include <SDL2/SDL.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "wasm3.h"
#include "assets.h"

#define MAX_IMAGES 8
#define MAX_TINTS 256
#define UPDATE_HZ 30

typedef struct {
    IM3Function init;
    IM3Function update;
    IM3Function render;
    IM3Function mousemove;
    IM3Function mousedown;
    IM3Function key;
    IM3Function quit;
    IM3Function set_portrait;
    IM3Function width;
    IM3Function height;
    IM3Function hover;
} GameFuncs;

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
    GameFuncs funcs;
} App;

static App app;
static char text_chars[256];
static int text_count;
static Uint32 critical_start;
static int critical_active;

static void die(M3Result result, const char *what)
{
    if (result) {
        fprintf(stderr, "%s: %s\n", what, result);
        exit(1);
    }
}

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
        if (pixels[i] > 200 && pixels[i + 1] > 200 && pixels[i + 2] < 128) {
            pixels[i] = 255;
            pixels[i + 1] = 255;
            pixels[i + 2] = 0;
            continue;
        }
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

m3ApiRawFunction(host_sound)
{
    m3ApiSuccess();
}

m3ApiRawFunction(host_alpha)
{
    m3ApiGetArg(float, value);
    app.alpha = value;
    if (app.alpha < 0.0f) {
        app.alpha = 0.0f;
    } else if (app.alpha > 1.0f) {
        app.alpha = 1.0f;
    }
    m3ApiSuccess();
}

m3ApiRawFunction(host_color)
{
    m3ApiGetArg(int32_t, r);
    m3ApiGetArg(int32_t, g);
    m3ApiGetArg(int32_t, b);
    app.cr = (uint8_t)r;
    app.cg = (uint8_t)g;
    app.cb = (uint8_t)b;
    m3ApiSuccess();
}

m3ApiRawFunction(host_rect)
{
    m3ApiGetArg(float, x);
    m3ApiGetArg(float, y);
    m3ApiGetArg(float, w);
    m3ApiGetArg(float, h);
    Uint8 a = (Uint8)(app.alpha * 255.0f);
    if (y >= (float)app.lh - 64.0f && w >= (float)app.lw - 20.0f && h <= 48.0f) {
        m3ApiSuccess();
    }
    SDL_SetRenderDrawColor(app.renderer, app.cr, app.cg, app.cb, a);
    SDL_Rect rect;
    rect.x = (int)roundf(x);
    rect.y = (int)roundf(y);
    rect.w = (int)roundf(w);
    rect.h = (int)roundf(h);
    SDL_RenderFillRect(app.renderer, &rect);
    m3ApiSuccess();
}

m3ApiRawFunction(host_cls)
{
    m3ApiGetArg(int32_t, r);
    m3ApiGetArg(int32_t, g);
    m3ApiGetArg(int32_t, b);
    SDL_SetRenderDrawColor(app.renderer, (Uint8)r, (Uint8)g, (Uint8)b, 255);
    SDL_RenderClear(app.renderer);
    m3ApiSuccess();
}

m3ApiRawFunction(host_img)
{
    m3ApiGetArg(int32_t, id);
    m3ApiGetArg(int32_t, frame);
    m3ApiGetArg(float, x);
    m3ApiGetArg(float, y);
    m3ApiGetArg(float, rot);
    m3ApiGetArg(float, sx);
    m3ApiGetArg(float, sy);
    if (getenv("SHOGUN_DEBUG")) {
        fprintf(stderr, "img id=%d frame=%d x=%f y=%f sx=%f sy=%f color=%d,%d,%d\n", id, frame, x, y, sx, sy, app.cr, app.cg, app.cb);
    }
    if (id == 7 && frame >= 0 && frame < 96 && text_count < (int)sizeof(text_chars)) {
        char ch = (char)(frame + 32);
        if (ch >= 'A' && ch <= 'Z') {
            text_chars[text_count++] = ch;
        }
    }
    if (id == 5) {
        m3ApiSuccess();
    }
    if (id < 0 || id >= MAX_IMAGES || id >= (int)g_image_count) {
        m3ApiSuccess();
    }
    SDL_Texture *tex = tinted_texture(id);
    if (!tex) {
        m3ApiSuccess();
    }
    const ShogunImage *img = &g_images[id];
    int cols = img->frame_w ? img->w / img->frame_w : 1;
    int rows = img->frame_h ? img->h / img->frame_h : 1;
    if (cols < 1) {
        cols = 1;
    }
    if (rows < 1) {
        rows = 1;
    }
    int total = cols * rows;
    int f = frame % total;
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
    m3ApiSuccess();
}

m3ApiRawFunction(host_save)
{
    m3ApiGetArg(int32_t, value);
    save_settings(value);
    m3ApiSuccess();
}

static void link_host(IM3Module module)
{
    die(m3_LinkRawFunction(module, "env", "P_sound", "v(ii)", &host_sound), "link P_sound");
    die(m3_LinkRawFunction(module, "env", "P_alpha", "v(f)", &host_alpha), "link P_alpha");
    die(m3_LinkRawFunction(module, "env", "P_color", "v(iii)", &host_color), "link P_color");
    die(m3_LinkRawFunction(module, "env", "P_rect", "v(ffff)", &host_rect), "link P_rect");
    die(m3_LinkRawFunction(module, "env", "P_cls", "v(iii)", &host_cls), "link P_cls");
    die(m3_LinkRawFunction(module, "env", "P_img", "v(iifffff)", &host_img), "link P_img");
    die(m3_LinkRawFunction(module, "env", "P_save", "v(i)", &host_save), "link P_save");
}

static void find_func(IM3Function *out, IM3Runtime runtime, const char *name)
{
    die(m3_FindFunction(out, runtime, name), name);
}

static int32_t call_i32(IM3Function func, const char *name)
{
    int32_t result = 0;
    die(m3_CallV(func), name);
    die(m3_GetResultsV(func, &result), name);
    return result;
}

static void call_i(IM3Function func, const char *name, int32_t arg)
{
    die(m3_CallV(func, arg), name);
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
    call_i(app.funcs.set_portrait, "G_set_portrait", h > w ? 1 : 0);
    app.lw = call_i32(app.funcs.width, "G_width");
    app.lh = call_i32(app.funcs.height, "G_height");
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

typedef struct {
    char ch;
    uint8_t rows[7];
} Glyph;

static const Glyph g_font[] = {
    {' ', {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {'a', {0x00, 0x0E, 0x01, 0x0F, 0x11, 0x13, 0x0D}},
    {'b', {0x10, 0x10, 0x16, 0x19, 0x11, 0x11, 0x1E}},
    {'c', {0x00, 0x0F, 0x10, 0x10, 0x10, 0x10, 0x0F}},
    {'d', {0x01, 0x01, 0x0D, 0x13, 0x11, 0x11, 0x0F}},
    {'e', {0x00, 0x0E, 0x11, 0x1F, 0x10, 0x10, 0x0F}},
    {'A', {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'H', {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'I', {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1F}},
    {'L', {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}},
    {'R', {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}},
    {'T', {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},
    {'!', {0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04}},
    {'m', {0x00, 0x1A, 0x15, 0x15, 0x15, 0x15, 0x15}},
    {'n', {0x00, 0x16, 0x19, 0x11, 0x11, 0x11, 0x11}},
    {'o', {0x00, 0x0E, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {'r', {0x00, 0x17, 0x18, 0x10, 0x10, 0x10, 0x10}},
    {'s', {0x00, 0x0F, 0x10, 0x0E, 0x01, 0x11, 0x0E}},
    {'y', {0x00, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E}},
    {'C', {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}},
};

static const Glyph *find_glyph(char ch)
{
    for (size_t i = 0; i < sizeof(g_font) / sizeof(g_font[0]); ++i) {
        if (g_font[i].ch == ch) {
            return &g_font[i];
        }
    }
    return &g_font[0];
}

static void draw_text(const char *text, int x, int y, int scale, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    const int advance = 6 * scale;
    SDL_SetRenderDrawColor(app.renderer, r, g, b, a);
    for (const char *p = text; *p; ++p) {
        const Glyph *glyph = find_glyph(*p);
        int cx = x + (int)(p - text) * advance;
        for (int row = 0; row < 7; ++row) {
            for (int col = 0; col < 5; ++col) {
                if (glyph->rows[row] & (1u << (4 - col))) {
                    SDL_Rect rect = {cx + col * scale, y + row * scale, scale, scale};
                    SDL_RenderFillRect(app.renderer, &rect);
                }
            }
        }
    }
}

static int contains_text(const char *needle)
{
    size_t len = strlen(needle);
    if ((size_t)text_count < len) {
        return 0;
    }
    for (int i = 0; i + (int)len <= text_count; ++i) {
        if (memcmp(text_chars + i, needle, len) == 0) {
            return 1;
        }
    }
    return 0;
}

static void start_critical(Uint32 now)
{
    if (critical_active && now - critical_start < 2800) {
        return;
    }
    critical_active = 1;
    critical_start = now;
}

static void draw_critical(Uint32 now)
{
    if (!critical_active) {
        return;
    }
    Uint32 elapsed = now - critical_start;
    if (elapsed > 2800) {
        critical_active = 0;
        return;
    }
    float progress = (float)elapsed / 2800.0f;
    float fade = elapsed < 2200 ? 1.0f : 1.0f - (float)(elapsed - 2200) / 600.0f;
    if (fade < 0.0f) {
        fade = 0.0f;
    }
    float shake = elapsed < 900 ? sinf((float)elapsed * 0.09f) * 7.0f * (1.0f - progress) : 0.0f;
    SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_BLEND);
    if (elapsed < 180) {
        SDL_SetRenderDrawColor(app.renderer, 255, 255, 255, (Uint8)(220 * (180 - elapsed) / 180));
        SDL_RenderFillRect(app.renderer, NULL);
    } else {
        SDL_SetRenderDrawColor(app.renderer, 110, 0, 0, (Uint8)(70 * fade));
        SDL_RenderFillRect(app.renderer, NULL);
    }
    SDL_SetRenderDrawColor(app.renderer, 255, 40, 40, (Uint8)(220 * fade));
    for (int i = 0; i < 6; ++i) {
        int x0 = (int)((float)app.lw * 0.08f + (float)i * app.lw / 9.0f + shake);
        int x1 = x0 + app.lh / 2;
        for (int k = -3; k <= 3; ++k) {
            SDL_RenderDrawLine(app.renderer, x0 + k, 0, x1 + k, app.lh);
        }
    }
    SDL_SetRenderDrawColor(app.renderer, 255, 255, 255, (Uint8)(150 * fade));
    for (int i = 0; i < 6; ++i) {
        int x0 = (int)((float)app.lw * 0.08f + (float)i * app.lw / 9.0f + shake);
        SDL_RenderDrawLine(app.renderer, x0, 0, x0 + app.lh / 2, app.lh);
    }
    SDL_Texture *logo = base_texture(0);
    if (logo) {
        float scale = elapsed < 350 ? 0.35f + 0.65f * (float)elapsed / 350.0f
                                    : 1.0f + 0.08f * sinf((float)elapsed * 0.025f) * (1.0f - progress);
        float width = (float)g_images[0].w * scale;
        float height = (float)g_images[0].h * scale;
        SDL_FRect dst = {(float)app.lw / 2.0f - width / 2.0f + shake,
                         (float)app.lh / 2.0f - height / 2.0f + shake * 0.5f,
                         width, height};
        SDL_SetTextureAlphaMod(logo, (Uint8)(255 * fade));
        SDL_RenderCopyExF(app.renderer, logo, NULL, &dst, sinf((float)elapsed * 0.012f) * 3.0f, NULL, SDL_FLIP_NONE);
        SDL_SetTextureAlphaMod(logo, 255);
        SDL_SetRenderDrawColor(app.renderer, 255, 255, 0, (Uint8)(255 * fade));
        for (int i = 0; i < 12; ++i) {
            float angle = (float)i * 0.5236f + (float)elapsed * 0.006f;
            float radius = 90.0f + 18.0f * sinf((float)elapsed * 0.015f + (float)i);
            SDL_Rect spark = {(int)((float)app.lw / 2.0f + cosf(angle) * radius + shake),
                              (int)((float)app.lh / 2.0f + sinf(angle) * radius), 4, 4};
            SDL_RenderFillRect(app.renderer, &spark);
        }
    }
    const char *title = "CRITICAL HIT!";
    int title_scale = app.lw > 600 ? 5 : 3;
    int title_width = (int)strlen(title) * 6 * title_scale - title_scale;
    int title_y = app.lh / 2 + (app.lh > 600 ? 120 : 80) + (int)shake;
    draw_text(title, (app.lw - title_width) / 2 + (int)shake, title_y, title_scale,
              255, 230, 0, (Uint8)(255 * fade));
}

static void dump_frame(void)
{
    if (!getenv("SHOGUN_DUMP")) {
        return;
    }
    static int frame;
    if (++frame < 20) {
        return;
    }
    int w;
    int h;
    SDL_GetRendererOutputSize(app.renderer, &w, &h);
    size_t pitch = (size_t)w * 4;
    uint8_t *pixels = malloc(pitch * h);
    if (!pixels) {
        return;
    }
    int ok = SDL_RenderReadPixels(app.renderer, NULL, SDL_PIXELFORMAT_RGBA8888, pixels, (int)pitch);
    if (ok != 0) {
        free(pixels);
        return;
    }
    const char *path = getenv("SHOGUN_OUT");
    FILE *f = fopen(path ? path : "/tmp/opencode/nosound_probe.ppm", "wb");
    if (f) {
        fprintf(f, "P6\n%d %d\n255\n", w, h);
        for (int i = 0; i < w * h; ++i) {
            fwrite(pixels + (size_t)i * 4, 1, 3, f);
        }
        fclose(f);
    }
    free(pixels);
}

static void render_frame(Uint32 now)
{
    SDL_FRect rect;
    view_rect(&rect);
    SDL_SetRenderDrawColor(app.renderer, 0, 0, 0, 255);
    SDL_RenderClear(app.renderer);
    SDL_SetRenderTarget(app.renderer, app.target.tex);
    text_count = 0;
    die(m3_CallV(app.funcs.render, (int32_t)now), "G_render");
    if (getenv("SHOGUN_CRIT")) {
        start_critical(now);
    }
    if (contains_text("GAMEOVER") || contains_text("CHECKMATE") || contains_text("STALEMATE")) {
        start_critical(now);
    }
    draw_critical(now);
    SDL_SetRenderTarget(app.renderer, NULL);
    SDL_RenderCopyF(app.renderer, app.target.tex, NULL, &rect);
    dump_frame();
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
            call_i(app.funcs.key, "G_key", k);
            pad_repeat[k] = now + 350;
        } else if (state[k] && k <= 4 && now >= pad_repeat[k]) {
            call_i(app.funcs.key, "G_key", k);
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

    app.window = SDL_CreateWindow("SHOGUN - TCKSoft", SDL_WINDOWPOS_CENTERED,
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

    IM3Environment env = m3_NewEnvironment();
    if (!env) {
        fprintf(stderr, "m3_NewEnvironment failed\n");
        return 1;
    }
    IM3Runtime runtime = m3_NewRuntime(env, 262144, NULL);
    if (!runtime) {
        fprintf(stderr, "m3_NewRuntime failed\n");
        return 1;
    }
    IM3Module module = NULL;
    die(m3_ParseModule(env, &module, g_wasm, g_wasm_len), "m3_ParseModule");
    die(m3_LoadModule(runtime, module), "m3_LoadModule");
    link_host(module);
    die(m3_CompileModule(module), "m3_CompileModule");

    find_func(&app.funcs.init, runtime, "G_init");
    find_func(&app.funcs.update, runtime, "G_update");
    find_func(&app.funcs.render, runtime, "G_render");
    find_func(&app.funcs.mousemove, runtime, "G_mousemove");
    find_func(&app.funcs.mousedown, runtime, "G_mousedown");
    find_func(&app.funcs.key, runtime, "G_key");
    find_func(&app.funcs.quit, runtime, "G_quit");
    find_func(&app.funcs.set_portrait, runtime, "G_set_portrait");
    find_func(&app.funcs.width, runtime, "G_width");
    find_func(&app.funcs.height, runtime, "G_height");
    find_func(&app.funcs.hover, runtime, "G_hover_button");

    uint32_t seed = SDL_GetTicks() ^ (uint32_t)(uintptr_t)&app;
    die(m3_CallV(app.funcs.init, (int32_t)seed, load_settings(), (int32_t)0), "G_init");
    layout();
    const char *keys = getenv("SHOGUN_KEYS");
    if (keys) {
        for (const char *p = keys; *p; ++p) {
            if (*p >= '1' && *p <= '6') {
                call_i(app.funcs.key, "G_key", *p - '0');
            }
        }
    }

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
                    call_i(app.funcs.key, "G_key", code);
                }
                break;
            }
            case SDL_MOUSEMOTION: {
                float x;
                float y;
                to_logical(event.motion.x, event.motion.y, &x, &y);
                die(m3_CallV(app.funcs.mousemove, (double)x, (double)y), "G_mousemove");
                if (call_i32(app.funcs.hover, "G_hover_button")) {
                    SDL_SetCursor(SDL_GetDefaultCursor());
                }
                break;
            }
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    float x;
                    float y;
                    to_logical(event.button.x, event.button.y, &x, &y);
                    die(m3_CallV(app.funcs.mousedown, (double)x, (double)y), "G_mousedown");
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
            die(m3_CallV(app.funcs.update), "G_update");
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
    m3_FreeRuntime(runtime);
    m3_FreeEnvironment(env);
    SDL_DestroyRenderer(app.renderer);
    SDL_DestroyWindow(app.window);
    SDL_Quit();
    return 0;
}
