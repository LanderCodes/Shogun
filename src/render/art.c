#include "render/art.h"

#include "assets/assets.h"

#include <stdlib.h>
#include <string.h>

#define IMAGE_COUNT 8

static SDL_Texture *base_texture(Art *art, int id)
{
    if (id < 0 || id >= IMAGE_COUNT || id >= (int)g_image_count) return NULL;
    if (art->base[id]) return art->base[id];

    const ShogunImage *image = &g_images[id];
    SDL_Texture *tex = SDL_CreateTexture(art->renderer, SDL_PIXELFORMAT_RGBA8888,
                                         SDL_TEXTUREACCESS_STATIC,
                                         image->w, image->h);
    if (!tex) return NULL;
    SDL_UpdateTexture(tex, NULL, image->data, image->w * 4);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    art->base[id] = tex;
    return tex;
}

static SDL_Texture *tinted_texture(Art *art, int id)
{
    Color color = art->color;
    if (color.r == 255 && color.g == 255 && color.b == 255) {
        return base_texture(art, id);
    }

    for (int i = 0; i < art->tint_count; ++i) {
        Tint *t = &art->tints[i];
        if (t->id == id && t->color.r == color.r &&
            t->color.g == color.g && t->color.b == color.b) return t->texture;
    }

    const ShogunImage *image = &g_images[id];
    size_t bytes = (size_t)image->w * image->h * 4;
    uint8_t *pixels = malloc(bytes);
    if (!pixels) return NULL;
    memcpy(pixels, image->data, bytes);
    for (size_t i = 0; i < bytes; i += 4) {
        pixels[i]     = (uint8_t)((uint32_t)pixels[i] * color.r / 255);
        pixels[i + 1] = (uint8_t)((uint32_t)pixels[i + 1] * color.g / 255);
        pixels[i + 2] = (uint8_t)((uint32_t)pixels[i + 2] * color.b / 255);
    }

    SDL_Texture *tex = SDL_CreateTexture(art->renderer, SDL_PIXELFORMAT_RGBA8888,
                                         SDL_TEXTUREACCESS_STATIC,
                                         image->w, image->h);
    if (tex) {
        SDL_UpdateTexture(tex, NULL, pixels, image->w * 4);
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    }
    free(pixels);
    if (!tex) return NULL;

    Tint *slot;
    if (art->tint_count < 256) {
        slot = &art->tints[art->tint_count++];
    } else {
        slot = &art->tints[0];
        if (slot->texture) SDL_DestroyTexture(slot->texture);
    }
    slot->texture = tex;
    slot->id = id;
    slot->color = color;
    return tex;
}

void art_init(Art *art, SDL_Renderer *renderer)
{
    memset(art, 0, sizeof(*art));
    art->renderer = renderer;
    art->color = (Color){255, 255, 255};
    art->alpha = 1.0f;
}

void art_done(Art *art)
{
    for (int i = 0; i < IMAGE_COUNT; ++i) {
        if (art->base[i]) SDL_DestroyTexture(art->base[i]);
    }
    for (int i = 0; i < art->tint_count; ++i) {
        if (art->tints[i].texture) SDL_DestroyTexture(art->tints[i].texture);
    }
    memset(art, 0, sizeof(*art));
}

void art_clear(Art *art, Color color)
{
    SDL_SetRenderDrawColor(art->renderer, color.r, color.g, color.b, 255);
    SDL_RenderClear(art->renderer);
}

void art_rect(Art *art, float x, float y, float w, float h)
{
    SDL_SetRenderDrawColor(art->renderer, art->color.r, art->color.g, art->color.b,
                           (uint8_t)(art->alpha * 255.0f));
    SDL_FRect rect = {x, y, w, h};
    SDL_RenderFillRectF(art->renderer, &rect);
}

void art_image(Art *art, int id, int frame, float x, float y, float sx, float sy)
{
    art_image_rot(art, id, frame, x, y, 0.0f, sx, sy);
}

void art_image_rot(Art *art, int id, int frame, float x, float y,
                   float rot, float sx, float sy)
{
    SDL_Texture *tex = tinted_texture(art, id);
    if (!tex) return;

    const ShogunImage *image = &g_images[id];
    int cols = image->frame_w ? image->w / image->frame_w : 1;
    int rows = image->frame_h ? image->h / image->frame_h : 1;
    int total = cols * rows > 0 ? cols * rows : 1;
    int f = frame % total;
    if (f < 0) f += total;

    SDL_Rect src = {(f % cols) * image->frame_w, (f / cols) * image->frame_h,
                    image->frame_w, image->frame_h};
    SDL_FRect dst = {x - (float)image->frame_w * sx / 2.0f,
                     y - (float)image->frame_h * sy / 2.0f,
                     (float)image->frame_w * sx,
                     (float)image->frame_h * sy};
    SDL_SetTextureAlphaMod(tex, (uint8_t)(art->alpha * 255.0f));
    SDL_RenderCopyExF(art->renderer, tex, &src, &dst, rot, NULL, SDL_FLIP_NONE);
}
