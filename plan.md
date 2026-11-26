# Plan

## Backgrounds

- Use the checkerboard pattern **only on the menu** (including the title screen).
- Do **not** use the checkerboard on the game itself: play, pause, and winner screens.
- Restore the **blue background** on the game itself (play, pause, and winner screens).
- Keep text inside boxes and readable.

## Directory layout

- Make the refactor the repository root (per follow-up): previous root files move to `original/` and the old `test/` builds to `alt/`.
- Split the source into logical subdirectories, for example:
  - `refactor/src/core/` — board, rules, AI, game state
  - `refactor/src/render/` — renderer, effects, font, layout
  - `refactor/src/ui/` — menu, input
  - `refactor/src/assets/` — embedded image data
  - `refactor/tests/` — logic tests
- Update build scripts, includes, and documentation to match the new layout.
- Keep the pure C99 rewrite silent: no WASM runtime, no sound data, and no scrolling ticker text.

## Menu credit

- Add a small credit on the menu: `Made by LanderCodes`.
- Place it so it does not cover the logo, menu boxes, or setting labels.

## Special characters

- Fix special character rendering in the refactor font/text path.
- Audit glyph mapping for punctuation and non-ASCII characters used by the UI.
- Replace or map unsupported characters to valid glyphs so no wrong or blank symbols are drawn.
- Re-check all menu, help, status, and winner text after the change.

## Verification

- Build the refactor and run its tests.
- Run a dummy-SDL smoke test.
- Confirm the menu still shows the checkerboard and the game screens use the blue background.
- Confirm the winner rematch menu still works.
- Confirm no `critical hit` text appears anywhere in the UI.
- Run `git diff --check` before committing; do not push.
