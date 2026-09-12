# Contributing

Bug reports and small, focused fixes are welcome. Please include the game edition, browser/OS, steps to reproduce, expected and actual behavior, and a screenshot when useful. Say whether you are testing corncob3d.com or your own build and include the commit when known.

For a code change, fork the repository, create a branch, build with `python tools/build.py`, and run `npm test`. Test affected menus and flight behavior in a real browser. For web-only changes after an initial build, `python tools/build_web.py` is sufficient to refresh the package.

Explain the problem, what changed and how you checked it. For source-fidelity changes, cite the original routine, data, documentation or a reproducible original-runtime comparison. Preserve fixed-width integer behavior and operation ordering. An intentional gameplay modification is welcome as an explicitly identified experiment; do not call it an original-game fix without evidence.

Keep original asset bytes unchanged and preserve licensing/provenance. Do not contribute music or assets without the appropriate rights. Do not add tokens, personal files, exported saves, build output, executables or large research archives. Sanitized synthetic save fixtures are preferable to personal saves.

Public issues are for ordinary bugs. See SECURITY.md for sensitive reports. No contribution should silently enable telemetry, upload local files, change the live site, or require account credentials.

The included tests cover a bounded public subset of the project's checks. They do not prove complete game parity, complete security, or equivalent browser behavior.
