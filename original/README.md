# SHOGUN - silent Linux SDL2 builds

This repository contains three Linux SDL2 builds of the same SHOGUN game. All are
silent: there is no audio device, no sound assets and no sound playback in any
build.

1. the repository root - a from-scratch, plain C99 rewrite (recommended for
   future work; no WebAssembly, no generated code);
2. `alt/full-c-no-sound` - the native C port generated with WABT's `wasm2c`
   (no interpreter dependency);
3. `alt/no-sound` - the earlier wasm3 interpreter build, kept for reference.

This file is the historical overview of all three builds. The rewrite is now the
root project and is documented in the root `README.md`.

The two older builds replay the original WebAssembly module, either interpreted
directly or translated into portable C by `wasm2c`, driven by a small SDL2 host
program that implements the imports the module expects. The rewrite implements
the same game directly in ordinary C.

The upstream game is SHOGUN by TCKSoft / Andrew Pointon, based on the board game
by Teruo Matsumoto. See `original/README.txt` for the original game rules,
controls and credits.

---

## 1. Repository structure

```text
/home/lander/Projects/game/
├── README.md                  root project documentation
├── CMakeLists.txt             clean pure-C rewrite build (recommended)
├── src/                       core/, render/, ui/ and assets/
├── tests/                     logic and dummy-SDL smoke tests
├── plan.md                    change plan
├── original/                  historical files from the old repository root
│   ├── README.md              this file
│   ├── README.txt             original upstream game notes
│   ├── Shogun.exe             pre-existing Windows executable (opaque binary)
│   └── no-sound/              earlier no-sound copy
└── alt/                       alternative silent builds
    ├── full-c-no-sound/       native C port, silent
    │   ├── CMakeLists.txt
    │   ├── README.txt
    │   ├── WABT-LICENSE
    │   ├── build/             generated CMake build directory
    │   └── src/
    │       ├── assets.h       embedded image arrays only
    │       ├── game.c         wasm2c-generated game logic (~14,937 lines)
    │       ├── game.h         generated declarations and import prototypes
    │       ├── main.c         SDL2 host
    │       └── wasm-rt*       WABT C runtime
    └── no-sound/              legacy wasm3-based silent build
        ├── CMakeLists.txt
        ├── README.txt
        ├── build/             generated CMake build directory
        ├── src/
        │   ├── assets.h       embedded WASM bytes and image arrays
        │   └── main.c         wasm3-based SDL2 host
        └── third_party/
            └── wasm3/         vendored wasm3 interpreter
```

---

## 2. Architecture

Every build has the same split:

```text
game logic (WASM module or generated C)  <-->  host main.c (SDL2 + platform)
```

The game module owns:

- board state, piece movement, hidden terrain and AI;
- menu state, animation timers, win detection;
- which images, rectangles and colors should be drawn;
- settings values it asks the host to save.

The host program owns:

- the SDL window, renderer and render target;
- loading image bytes from C arrays;
- converting module draw calls into SDL drawing calls;
- keyboard, mouse and controller input translation;
- the fixed-step update loop;
- config file persistence;
- fullscreen toggling and resizing.

The module never calls SDL directly. It calls a tiny import API:

| Import | Purpose |
| --- | --- |
| `P_img(id, frame, x, y, rot, sx, sy)` | draw a frame from an image atlas, centered at `x,y` |
| `P_rect(x, y, w, h)` | draw a filled rectangle using the current color/alpha |
| `P_cls(r, g, b)` | clear the logical framebuffer |
| `P_color(r, g, b)` | set the current tint color |
| `P_alpha(a)` | set the current alpha |
| `P_sound(id, channel)` | part of the original ABI; a no-op stub in both builds |
| `P_save(value)` | persist the settings bitfield |

The generated module exports:

| Export | Purpose |
| --- | --- |
| `G_init(seed, settings, extra)` | initialize a new game |
| `G_update()` | advance one logic tick |
| `G_render(now)` | issue draw calls for the current frame |
| `G_mousemove(x, y)` | send pointer position |
| `G_mousedown(x, y)` | send pointer click |
| `G_key(code)` | send a menu/game key |
| `G_quit()` | returns whether the app should exit |
| `G_set_portrait(flag)` | switch between landscape and portrait layouts |
| `G_width()` / `G_height()` | logical drawing resolution |
| `G_hover_button()` | used for pointer feedback |
| `G_autoplay(flag)` | generated export used only by the native C build |

---

## 3. Game rules and controls

From `original/README.txt`:

- Capture the enemy SHOGUN, or leave the enemy with only its SHOGUN and one pawn.
- Hidden terrain under a piece determines its exact move distance:
  - pawns move 1-4 squares;
  - SHOGUN moves 1 or 2 squares.
- Pieces cannot move diagonally, jump, double back, or turn more than once per move.
- Landing on an enemy piece captures it.

Controls:

- Mouse: click a piece, then click a destination; click the piece again to cancel.
- Keyboard: arrows/WASD move the cursor; Enter/Space selects.
- Controller: d-pad or left stick moves; A selects; B or Start pauses.
- Escape: pause, then main menu; quits from the main menu.
- F11 or Alt+Enter toggles fullscreen.

Both builds use the logical resolutions reported by the game module:

- landscape: `960x540`
- portrait: `480x800`

The window itself can be resized. Rendering happens into a logical-size SDL
texture and is then copied to the window with a letterboxed nearest-neighbor
scale.

The original scrolling welcome message is never drawn. In the root rewrite the
title and menu use the checkerboard backdrop and the menu carries the small
`Made by LanderCodes` credit; play, pause and winner screens use the blue
backdrop. The builds in `alt/` keep their older presentation.

---

## 4. Native C port (`alt/full-c-no-sound`)

This is the generated C port. `src/assets.h` contains only the image tables;
all sound arrays are gone. `src/main.c` does not initialize SDL audio, does not
open an audio device, and `w2c_env_P_sound()` is an empty stub.

`w2c_env_P_img()` suppresses the `gfxtext` frame 2 draw call, and it also filters
the game's bottom menu ticker. That ticker is not a `gfxtext` frame: the module
draws it character by character with `zxfont` glyphs (`id == 7`) in a narrow band
just above the bottom bar (`y` around 38-40 pixels above the logical height).
The host drops those specific glyph draws and draws nothing in their place.
Other text rows, including the pause/menu hint and the help screen, are outside
that band and remain untouched. The host also drops the two full-width
`P_rect` fills that form the ticker's background strip. The legacy host filters
the same ticker glyphs (its existing bottom-bar rule already skips the strip)
and no longer draws its own credit line.

The host also skips the bottom 8-pixel bar drawn by the game, matching the
earlier experimental no-sound behavior.

### How the C port was made

1. The embedded WASM module was extracted from the old `src/assets.h` into a
   standalone `.wasm` file.
2. WABT 1.0.42 translated it with `wasm2c -n shogun game.wasm -o game.c`,
   producing `game.c` and `game.h`. (`-n`/`--module-name` sets the prefix in that
   version; `--prefix=` is not valid.)
3. The WABT C runtime files (`wasm-rt.h`, `wasm-rt-impl.c`, `wasm-rt-impl.h`,
   `wasm-rt-impl-tableops.inc`, `wasm-rt-mem-impl.c`,
   `wasm-rt-mem-impl-helper.inc`) were copied next to `game.c`.
4. A new `main.c` implements the `env` imports:
   - `base_texture(id)` uploads an RGBA8888 image from `g_images[id]` once;
   - `tinted_texture(id)` multiplies image RGB bytes by the current `P_color`;
   - `draw_frame(...)` selects an atlas frame, applies rotation/scale/alpha;
   - `layout()` and `view_rect()` handle logical-to-window scaling;
   - a 30 Hz fixed-step update loop uses a 250 ms accumulator cap;
   - input is translated to the integer codes expected by `G_key`.

The generated code keeps its own linear memory (`instance->w2c_memory.data`) and
reproduces the module's data segment, so it behaves like the original WASM
without a WASM VM.

### Custom behavior

| Variable | Effect |
| --- | --- |
| `SHOGUN_DEBUG` | prints the random seed at startup |
| `SHOGUN_CRITICAL_HIT` | enables the synthetic "critical hit deaths" simulation and red overlay |
| `SHOGUN_AUTOPLAY` | passes `1` as the third argument to `G_init`, enabling the generated autoplay path |

Example:

```bash
SHOGUN_DEBUG=1 SHOGUN_CRITICAL_HIT=1 SHOGUN_AUTOPLAY=1 \
  ./alt/full-c-no-sound/build/shogun
```

The critical-hit overlay is active only while a game is in progress (state `1`),
so it cannot hide the winner banner (state `4`).

---

## 5. Legacy wasm3 build (`alt/no-sound`)

This build keeps the original WebAssembly module in `src/assets.h` and runs it
through the vendored wasm3 interpreter. Its `assets.h` has the image tables but
no sound arrays, and its host never calls `SDL_Init(SDL_INIT_AUDIO)`; the
`P_sound` import is an empty stub. Its host-drawn bottom credit line was removed,
it ignores all `gfxtext` (`id == 5`) draw calls, and it filters the `zxfont`
ticker glyphs (`id == 7`) in the bottom menu band, just like the native C host.

It is kept for reference only; use the root rewrite or `alt/full-c-no-sound` for
further work.

---

## 5b. Pure-C rewrite (repository root)

This project starts from the original game's image assets and rules but replaces
all game logic with ordinary C99. There is no generated code, no WebAssembly
runtime, no `P_img` import layer and no hidden memory addresses. Each feature
lives in a logical subdirectory:

```text
src/
├── core/       board, rules, AI, game state and settings
├── render/     renderer, effects, font/glyph mapping and layout
├── ui/         app shell, input and menu
└── assets/     embedded images only
```

The rewrite keeps the same logical resolutions (960x540 landscape, 480x800
portrait), menu options, controls and board rules. It has no sound and never
draws the original scrolling welcome text. See the root `README.md` for build
instructions and the list of intentional differences from the generated builds.

---

## 6. Images, atlases and fonts

`assets.h` defines `ShogunImage` entries with this shape:

```c
typedef struct {
    const uint8_t *data;
    uint32_t len;
    int w;
    int h;
    int frame_w;
    int frame_h;
    int scale;
} ShogunImage;
```

Image IDs:

| ID | Image | Frame size | Notes |
| --- | --- | --- | --- |
| 0 | logo | 282x92 | title logo |
| 1 | board | 60x60 | board terrain/pieces base |
| 2 | red pieces | 60x60 | red army |
| 3 | white pieces | 60x60 | white army |
| 4 | numbers | 60x60 | movement numbers |
| 5 | gfxtext | 384x64 | five stylized text frames, including winner banners |
| 6 | gfxhelp | 453x455 | help screen |
| 7 | zxfont | 16x16 | pixel font used for normal text |

Frame selection in the native C host is row-major:

```c
cols = img->w / img->frame_w;
rows = img->h / img->frame_h;
f = frame % (cols * rows);
src.x = (f % cols) * img->frame_w;
src.y = (f / cols) * img->frame_h;
```

This matters for `gfxtext`: it has one column and five rows, so winner banners
are frames 3 and 4 rather than crops of the top row.

---

## 7. Important internal addresses

These addresses are in the generated module's linear memory. They were
discovered by inspecting the generated C and by runtime tracing.

| Address | Meaning |
| --- | --- |
| `5440` | settings bitfield passed to `G_init` |
| `5456` | current game state |
| `5460` | random seed |
| `5472` | portrait/layout flag |
| `5488` | quit flag read by `G_quit` |
| `5492` | menu cursor/selection state |
| `6032` | ASCII-to-glyph font map |
| `6468` | winner value on the winner screen |

Settings bits:

| Bit | Meaning |
| --- | --- |
| 0 | show move guides |
| 1 | always show terrain |
| 2 | improved computer opponent |

If no config file is present, `load_settings()` returns `-1`, which the game
treats as "use defaults".

Config path:

- `$XDG_CONFIG_HOME/tcksoft/shogun.cfg` when `XDG_CONFIG_HOME` is set;
- otherwise `$HOME/.config/tcksoft/shogun.cfg`.

---

## 8. How to build

### Dependencies

On Debian/Ubuntu:

```bash
sudo apt-get install build-essential cmake pkg-config libsdl2-dev
```

A C99 compiler and `libm` are also required. WABT is only needed if you want to
regenerate `game.c`; it is not needed to build the checked-in C port.

### Pure-C rewrite (recommended)

```bash
cd /home/lander/Projects/game
cmake -S . -B build
cmake --build build -j2
ctest --test-dir build --output-on-failure
./build/shogun
```

### Native C port

```bash
cd /home/lander/Projects/game
cmake -S alt/full-c-no-sound -B alt/full-c-no-sound/build
cmake --build alt/full-c-no-sound/build -j2
./alt/full-c-no-sound/build/shogun
```

### Legacy wasm3 build

```bash
cd /home/lander/Projects/game
cmake -S alt/no-sound -B alt/no-sound/build
cmake --build alt/no-sound/build -j2
./alt/no-sound/build/shogun
```

### Regenerating `game.c`

If you obtain a new `game.wasm` that matches the same import/export ABI:

```bash
wasm2c -n shogun game.wasm -o alt/full-c-no-sound/src/game.c
```

Then copy the newly generated `game.h` next to it and re-copy the WABT runtime
files if needed. Re-apply the silent-host changes; do not assume the internal
memory addresses listed above still apply to a different WASM build.

---

## 9. Known weak points

1. **`game.c` in `alt/full-c-no-sound` is generated and huge.** It is about
   14,937 lines of machine-generated control flow; the pure-C rewrite is the
   better place for logic changes.
2. **Internal addresses are hard-coded in the host.** The native C host reads
   `5456` for game state and knows about `6468` for winner values. A different
   WASM build may move them. The menu ticker filter is likewise tied to the
   current logical layout.
3. **Only the rewrite has automated tests.** `ctest` in `build/` checks game
   logic, glyph mapping and dummy-SDL rendering; the older builds were checked
   with headless smoke tests and temporary trace tools outside the repo.
4. **The smoke test does not compare every rendered pixel.** It samples the
   menu checkerboard and game backgrounds; remaining visual checks need a real
   or virtual display.
5. **Settings persistence is minimal.** One signed integer is written as decimal
   text, with no versioning, validation or migration.
6. **`build/` directories are present.** They are generated artifacts, not
   source.
7. **`original/Shogun.exe` is opaque.** It is a pre-existing Windows binary;
   these READMEs document the Linux SDL2 source builds.

---

## 10. Quick reference

```bash
# Pure-C rewrite, silent (recommended)
cmake -S . -B build
cmake --build build -j2
ctest --test-dir build --output-on-failure
./build/shogun

# Native C port, silent
cmake -S alt/full-c-no-sound -B alt/full-c-no-sound/build
cmake --build alt/full-c-no-sound/build -j2
SHOGUN_DEBUG=1 ./alt/full-c-no-sound/build/shogun

# Legacy wasm3 build, silent
cmake -S alt/no-sound -B alt/no-sound/build
cmake --build alt/no-sound/build -j2
./alt/no-sound/build/shogun
```
