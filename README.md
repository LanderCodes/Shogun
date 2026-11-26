# SHOGUN - clean pure-C rewrite

This is the repository root: a from-scratch, plain C99 implementation of SHOGUN.
It uses the original game's image assets, but **no WebAssembly, no wasm2c output,
no wasm3, no WABT runtime, no sound and no scrolling ticker text**.

The alternative WASM-based builds live in `alt/`; the original upstream files
live in `original/`.

## Layout

```text
.
├── CMakeLists.txt          build and test definitions
├── README.md               this file
├── plan.md                 change plan
├── src/
│   ├── core/               board, rules, AI, game state, settings
│   ├── render/             renderer, effects, font/glyph mapping, layout
│   ├── ui/                 app shell, input, menu
│   └── assets/             embedded image data only
├── tests/
│   ├── logic_test.c        headless game-logic and glyph checks
│   └── smoke_test.c        dummy-SDL rendering and rematch checks
├── original/               upstream README, executable and earlier no-sound copy
└── alt/                    generated C port and legacy wasm3 build
```

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

## Presentation

- The checkerboard backdrop is used only on the title and menu screens.
- Play, pause and winner screens use the original blue backdrop (`0, 0, 64`).
- Menu text, including the settings labels and the small `Made by LanderCodes`
  credit, is kept inside the screen and clear of the logo and menu boxes.
- The zxfont character-to-frame table was corrected: punctuation, digits,
  letters and brackets map to their real glyphs; unsupported input maps to `?`.
  No UI string draws a blank or unrelated symbol, and there is no critical-hit
  text anywhere in this build.

## Build and test

```bash
cmake -S . -B build
cmake --build build -j2
ctest --test-dir build --output-on-failure
./build/shogun
```

Requires a C99 compiler, CMake, pkg-config and SDL2. No audio libraries are
used and no sound device is opened. CTest runs the smoke test with the dummy SDL
video driver and software renderer, so it works without a display.

## Differences from the builds in `alt/`

The builds in `alt/` replay the original WebAssembly bytecode. This build
implements the observable game directly in ordinary C, so small details such as
AI move choice, exact menu animation timings and wording will not be
byte-identical. The assets, board rules, screen flow, input methods, absence of
sound and absence of ticker text are preserved.
