# SHOGUN - clean pure-C rewrite

This directory is a from-scratch, plain C99 implementation of SHOGUN. It uses the
original game's image assets, but **no WebAssembly, no wasm2c output, no wasm3,
and no WABT runtime**. There are no sound arrays and no scrolling text.

The code is deliberately split into small modules so each part of the game has
its own file:

| File | Responsibility |
| --- | --- |
| `src/main.c` | program entry point and fixed-step main loop |
| `src/app.c` | SDL window, renderer, logical render target, scaling, fullscreen |
| `src/input.c` | keyboard, mouse and controller translation |
| `src/game.c` | screen state machine, selection, turns, win detection |
| `src/menu.c` | menu item names and settings status text |
| `src/render.c` | draws every screen with the image atlases |
| `src/art.c` | SDL texture cache, tinted sprites, rectangles and clears |
| `src/font.c` | zxfont glyph mapping and text layout |
| `src/board.c` | new board, hidden terrain numbers and piece values |
| `src/rules.c` | legal move generation, check and winner tests |
| `src/ai.c` | computer player (steady classic, deep-search improved) |
| `src/layout.c` | landscape/portrait geometry and hit testing |
| `src/config.c` | fresh session-only settings (nothing saved) |
| `src/effects.c` | check badge, winner targeting, spin-in shogun and logo impact |
| `src/types.h` | shared plain structs and enums |
| `src/assets.h` | the original embedded image tables (data only) |

## Rules implemented

- An 8x8 board with hidden terrain numbers from 1 to 4.
- A pawn moves exactly the number of squares shown on it; a SHOGUN moves 1 or 2.
- Orthogonal moves only: no diagonals, no jumping, no doubling back, at most one
  turn per move. The number changes to the terrain value of the landing square.
- Landing on an enemy piece captures it.
- A side loses when its SHOGUN is captured or when it is left with only its
  SHOGUN and one pawn.

## Controls

- Mouse: click a piece, then a destination; click the piece again to cancel.
- Keyboard: arrows/WASD move the cursor, Enter/Space selects, Escape pauses.
- Controller: d-pad/left stick moves, A selects, B or Start pauses.
- F11, Alt+Enter or F toggles fullscreen.

## Build

```bash
cmake -S test/refactor -B test/refactor/build
cmake --build test/refactor/build -j2
./test/refactor/build/shogun
```

Requires a C99 compiler, CMake, pkg-config and SDL2. No audio libraries are
used and no sound device is opened.

## Differences from the generated builds

The generated builds replay the original WebAssembly bytecode. This build
implements the observable game directly in ordinary C, so small details such as
AI move choice, exact menu animation timings and wording will not be
byte-identical. The assets, board rules, screen flow, input methods and the
absence of sound and ticker text are preserved.
