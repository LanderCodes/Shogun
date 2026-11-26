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
├── online/                 self-contained build with two-player online mode
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

## Install and build

You need a C99 compiler, CMake 3.16+, pkg-config/pkgconf, SDL2 and Git. No audio
libraries are used and no sound device is opened.

Clone the repository:

```bash
git clone https://github.com/LanderCodes/Shogun.git
cd Shogun
```

### Linux

Debian/Ubuntu:

```bash
sudo apt update
sudo apt install build-essential cmake pkg-config libsdl2-dev git
```

Fedora:

```bash
sudo dnf install gcc cmake pkgconf-pkg-config SDL2-devel git
```

Build and test both the single-player/root build and the online build:

```bash
cmake -S . -B build
cmake --build build -j2
ctest --test-dir build --output-on-failure

cmake -S online -B online/build
cmake --build online/build -j2
ctest --test-dir online/build --output-on-failure
```

Run them:

```bash
./build/shogun
./online/build/shogun
```

CTest runs smoke tests with the dummy SDL video driver and software renderer,
so the tests work without a display.

### Windows

You do not need `pacman`, Git, CMake or a compiler just to play. Use a prepared
Windows folder containing:

```text
shogun.exe
SDL2.dll
```

For online play with a hosted server, the host also needs:

```text
shogun-server.exe
SDL2.dll
```

Keep the `.exe` and `SDL2.dll` in the same folder. Double-click `shogun.exe` to
play. If Windows says `SDL2.dll` is missing, put `SDL2.dll` next to `shogun.exe`.

To host from Command Prompt in that folder:

```bat
set SHOGUN_TOKEN=your-private-token
shogun-server.exe
```

Leave that window open while people are playing. Building from source on Windows
needs a C99/SDL2 toolchain; the instructions above are for installing and
playing the prepared Windows build.

### Online setup

The `online/` build adds `TWO PLAYERS` -> `ONLINE GAME`. The setup menu asks only
for the access token; the server address is compiled into the client and shown
on the setup screen. The server binary is `online/build/shogun-server` on Linux
and `shogun-server.exe` in the Windows folder. See `online/README.md` for
server, token, VPN/port-forward and troubleshooting instructions, and
`online/notes.md` for implementation notes.

## Differences from the builds in `alt/`

The builds in `alt/` replay the original WebAssembly bytecode. This build
implements the observable game directly in ordinary C, so small details such as
AI move choice, exact menu animation timings and wording will not be
byte-identical. The assets, board rules, screen flow, input methods, absence of
sound and absence of ticker text are preserved.
