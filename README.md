# Corncob 3D — browser source port

[Play at corncob3d.com](https://corncob3d.com/) · [Report a bug](https://github.com/whiteyt223/corncob3d/issues) · [Credits and permissions](LICENSE.md)

A fan preservation project bringing Corncob 3D to desktop browsers. The original game is by **Kevin Stokes, George Welch and Pie in the Sky Software**, published by MVP Software. This project was developed with AI coding assistance.

The game uses a source-derived C++ simulation and software renderer compiled to WebAssembly, with JavaScript browser adapters and HTML menus. It is not a DOS emulator running the original executable. The code follows recovered source and measured original behavior; it does **not** claim perfect whole-game parity.

Included editions: Shareware 3.42 and Other Worlds. There is no separately verified full registered/Deluxe edition. Original music files are not included; the game can read user-selected ROL/BNK files locally.

## Build and play

The tested build environment is Linux with **Python 3.12**, **Node.js 24**, a system **g++** compiler and the pinned Zig Python package. On Windows, use WSL with Ubuntu for compiling. No MASM, DOS installation or game download is required for the supported build.

```sh
python3 -m venv .venv
. .venv/bin/activate
python -m pip install -r tools/requirements.txt
python tools/build.py
npm test
npm run dev -- --host 127.0.0.1 --port 4173
```

Open **http://localhost:4173/**. Browsers normally treat localhost as trustworthy for audio; remote deployments need HTTPS. Click into the game to activate sound. `file://` is not supported. Node serves the static build; it is not a production game backend.

For JavaScript/CSS/HTML-only edits, run `python tools/build_web.py` after the first complete build. C++ or YMFM changes require `python tools/build.py` again. The build strips debugging sections from the shipped Wasm; the editable sources remain here. To regenerate the DOS font, install `fonttools` and run `python tools/build_menu_font.py`; ordinary builds use the included WOFF. `npm test` uses built Wasm and retained synthetic/game fixtures; it does not launch a browser.

## Where to experiment

| Directory | What it contains |
|---|---|
| `core/` | Translated fixed-point simulation, renderer, gameplay and original record handling |
| `web/` | Browser menus, input, Canvas display, local saves, audio bridge and import validation |
| `third_party/` | YMFM audio source, AdPlug derivation notices and DOSBox font license/provenance |
| `originals/` | Selected immutable original game inputs and preserved permission evidence |
| `reference/`, `research/` | Selected decoded/captured build inputs, preserving their original paths |
| `tools/` | Build, static server and focused regression checks |
| `docs/` | Architecture, release notes and data provenance |

Start with `web/style.css` for appearance, `web/menu.mjs` for menus, or `core/` for gameplay. Keep original arithmetic and update ordering intact unless a change is intentional and documented. See [CONTRIBUTING.md](CONTRIBUTING.md).

This is a curated, independently buildable snapshot of the current game. It excludes the older speculative prototype, internal hosting metadata, research executables/archives, imported music, personal saves and the large development evidence collection. The retained build inputs are listed with hashes in `docs/PUBLIC_INPUTS.json`. The much larger original-emulator comparison suite is not part of this checkout; do not mistake the small public test suite for full equivalence testing.

## Hosting and downloads

The GitHub Actions workflow builds the game, runs the focused checks, and produces a **cloudflare-pages** artifact. Download and extract it; upload its `dist` contents (including `_headers`) to your own static host. No GitHub workflow deploys to the live site or needs Cloudflare credentials. For Cloudflare's browser upload, `python tools/package_release.py` produces `build/corncob3d-cloudflare.zip` with the correct root layout.

Game saves and imported boss text stay in browser storage. Export saves before changing browsers or clearing site data. Exported saves include boss-screen text, so review them before attaching to a public issue.

## Rights and attribution

Please read [LICENSE.md](LICENSE.md) before redistributing. This repository preserves historical game notices and TideGear's later permission account. It does not relabel the entire game MIT/GPL or claim public-domain status. Third-party components retain their individual licenses. Do not add original ROL/BNK music or third-party assets without suitable permission.

Thanks to **TideGear** for preserving the games and documenting the permission account, and to **Jason Biniewski, Anatoly Shashkin and foone** for source preservation. YM3812 emulation uses **Aaron Giles's ymfm**; the ROL event player is derived from **AdPlug**; the DOS font data comes from **DOSBox**.
