# ymfm provenance

This directory contains the minimum source files required to build the YM3812
bridge from [Aaron Giles's ymfm repository](https://github.com/aaronsgiles/ymfm),
commit `81aec25ccbb98f4873a255f7551ac4dadac59b4a` (2026-07-27).

Files were fetched from the repository's `src/` directory without modification:
`ymfm.h`, `ymfm_fm.h`, `ymfm_fm.ipp`, `ymfm_opl.h`, `ymfm_opl.cpp`,
`ymfm_adpcm.h`, `ymfm_adpcm.cpp`, `ymfm_pcm.h`, and `ymfm_pcm.cpp`.

`LICENSE` is the upstream BSD 3-Clause License. The local
`ymfm_opl2_adapter.cpp` is separate project glue and does not alter the core.
