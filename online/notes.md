# SHOGUN - online subproject

This directory is a self-contained copy of the clean pure-C SHOGUN rewrite with
local and online two-player play added. The main game in `..` is untouched; build
and run this directory to try the online flow.

It uses the original game's image assets, but **no WebAssembly, no wasm2c output,
no wasm3, no WABT runtime, no sound and no scrolling ticker text**.

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
├── client.c                non-blocking client state machine
├── protocol.c              message parsing and name sanitising
├── server.c                matchmaking and move relay
├── server_main.c           standalone shogun-server entry point
├── socket_util.c           sockets, line buffering and LAN discovery
└── tests/
    ├── logic_test.c        headless game-logic and glyph checks
    ├── online_test.c       protocol and loopback matchmaking checks
    └── smoke_test.c        dummy-SDL rendering and rematch checks
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

## Online play

Select `TWO PLAYERS` and then `ONLINE GAME`. The setup menu asks only for the
access token and shows the compiled server address, then `NEXT` opens the name
entry. Type a name (letters, numbers, spaces, `_` and `-`) and press Enter. The
server address is a lightly obfuscated value in `client.c`, decoded at startup.
Only the token is stored in `~/.shogun_online.cfg` so it is prefilled next time;
on Unix the file is created with mode `600`.

Both the server and every client must use the same private token. The token is
never sent over the network; the server sends a random nonce and the client
replies with a hash of nonce + token. The `SHOGUN_TOKEN` environment variable
still works as an initial default for the token field.

Start a server on one machine before playing:

```bash
./online/build/shogun-server            # default port 7777
./online/build/shogun-server 9000       # custom port; rebuild clients with that address
```

For a systemd service, put the token in a root-only environment file such as
`/etc/shogun-server.env` (`chmod 600`) and add
`EnvironmentFile=/etc/shogun-server.env` to the service unit.

While online, Escape pauses; `MAIN MENU` forfeits the match. After a finished
game, `PLAY AGAIN` asks the opponent for a rematch; the server deals a new seed
once both players agree.

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

From the main game directory:

```bash
cmake -S online -B online/build
cmake --build online/build -j2
ctest --test-dir online/build --output-on-failure
./online/build/shogun
```

This builds `online/build/shogun` and `online/build/shogun-server`. Requires a C99
compiler, CMake, pkg-config and SDL2. No audio libraries are used and no sound
device is opened. CTest runs the smoke test with the dummy SDL video driver and
software renderer, so it works without a display; the online test starts its own
loopback server on port 7799.
