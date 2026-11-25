#ifndef SHOGUN_ART_H
#define SHOGUN_ART_H

#include <SDL2/SDL.h>

typedef struct { uint8_t r, g, b; } Color;

typedef struct {
    SDL_Texture *texture;
    int id;
    Color color;
} Tint;

typedef struct {
    SDL_Renderer *renderer;
    SDL_Texture *base[8];
    Tint tints[256];
    int tint_count;
    Color color;
    float alpha;
} Art;

void art_init(Art *art, SDL_Renderer *renderer);
void art_done(Art *art);
void art_clear(Art *art, Color color);
void art_rect(Art *art, float x, float y, float w, float h);
void art_image(Art *art, int id, int frame, float x, float y, float sx, float sy);
void art_image_rot(Art *art, int id, int frame, float x, float y,
                   float rot, float sx, float sy);

#endif
