# Licensing, attribution and preserved permission evidence

This is a source-available fan preservation project. There is no single blanket standard license asserted for all files.

## Original game and translated code

Corncob 3D is by Kevin Stokes / Pie in the Sky Software, with George Welch credited in the preserved permission account, and was published by MVP Software. Original source-derived logic, game data, artwork, SFX and documentation retain their authorship and notices. The historical distribution notice is preserved verbatim at `originals/shareware-v342/extracted/LICENSE.DOC`; the game manuals retain their original credits.

TideGear's preserved public account reports obtaining permission from Kevin Stokes and George Welch to distribute both games as freeware, and separately reports broad source reuse permission from Stokes. We retain the actual HTML captured by Common Crawl on 2018-05-20 and its indexed record:

- `originals/recovered-editions/tidegear-page-20180520.html`
- `originals/recovered-editions/commoncrawl-2018-22.jsonl`
- Original page: http://tidegear.net/corncob/index.html
- Recovered original source mirror: https://github.com/foone/Corncob3D

The HTML payload's recorded SHA1/Base32 digest is `WQ5QDF2DEY5364FC2XIKC2FMRXZCLFZA`. This is a preserved public permission account, not the underlying private correspondence, an OSI license identifier or a public-domain dedication. We preserve both the historical notice and the later account; no broader rights are invented for third-party material. The browser translation and adapters are provided with this provenance rather than presented as wholly original code under a blanket MIT license.

## Third-party components

| Component | Applicable notice |
|---|---|
| YMFM / YM3812 audio emulation, Aaron Giles and contributors | BSD-3-Clause; `third_party/ymfm/LICENSE` and `THIRD_PARTY.md` |
| JavaScript ROL/BNK event player derived from AdPlug | LGPL-2.1-or-later; `web/rol-music.mjs`, `third_party/adplug/LICENSE` and `PROVENANCE.md` |
| DOSBox-derived font data and generated WOFF | GPL-2.0-or-later; `third_party/dosbox-font/LICENSE` and `PROVENANCE.md`; editable glyphs in `web/iscore-font16.mjs`, generator in `tools/build_menu_font.py` |

The build tools' and GitHub Actions' licenses remain their own. These tools are not incorporated into the game merely by running a build.

## Original music excluded

The original manual credits James L. Collymore's music and imposes separate copying/use restrictions. ROL and BNK files, music-bearing distribution archives and original executable bundles are excluded from this repository. The browser supports local import of files a user already has. No permission to redistribute those music files is implied.

Retain notices and identify modifications when sharing a derivative. For commercial reuse or permissions beyond the preserved account, obtain clarification from the relevant rights holders. This document records provenance; it does not create rights on their behalf.
