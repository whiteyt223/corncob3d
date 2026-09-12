# Architecture and scope

`core/` translates the original fixed-point routines and emulated game state into C++. The freestanding Wasm module exposes `cc_*` entry points. JavaScript orchestration loads edition assets, dispatches input and presents the indexed software-rendered image on Canvas.

`web/startup.mjs`, `session.mjs` and `game.mjs` connect launch, simulation and career flow. `career.mjs` and `storage.mjs` manage original-format records and atomic IndexedDB writes. `menu.mjs` implements the browser menus; browser input, timing, fullscreen and focus handling live in the surrounding adapters.

SFX register writes reach the separate YMFM Wasm chip emulator through an AudioWorklet. The AdPlug-derived ROL player converts locally supplied music into register events. Original music is not downloaded or bundled.

The build retains separate Shareware and Other Worlds profiles, including distinct initial state, worlds, presentation and career behavior. Selected binary inputs are retained at their development paths for reproducibility. They are initialized/decoded game data, not a running DOS executable or a bundled emulator.

Current source includes the boss text privacy explanation, bounded imports, security headers, classic menu styling and sortie-report visibility repair. This public snapshot has focused build/import/session regression tests. Extensive historical frame/emulator captures are not included, and a passing test suite does not establish perfect fidelity or a complete security review. Real-device joystick, mobile and exhaustive cross-browser behavior remain outside these checks.

`docs/PUBLIC_INPUTS.json` maps retained upstream snapshot files to hashes. Treat those original bytes as immutable; add explicit transformations rather than silently replacing provenance.
