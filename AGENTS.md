# Working on this browser port

This checkout contains the current C++/WebAssembly and JavaScript game, not the old TypeScript prototype. Preserve original arithmetic, data provenance and licensing. Keep simulation changes tied to original source or reproducible behavior; label intentional departures clearly.

Build with `python tools/build.py`; for web-only changes after a full build use `python tools/build_web.py`. Run `npm test` for affected code/build changes and check visible UI changes in a browser. Do not claim a Node test is browser QA or whole-game parity. The full historical emulator evidence suite is not included here.

Do not commit tokens, imported music, personal saves, build outputs, downloaded research archives or internal hosting configuration. Do not deploy or contact people unless the user authorizes that action. Read LICENSE.md and CONTRIBUTING.md before adding assets or changing attribution.
