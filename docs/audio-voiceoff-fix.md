# Other Worlds repeating audio fix — 2026-09-12

A player recording showed a repeating high-pitched tone during takeoff. Toggling sound off/on did not resolve it. Spectral analysis found the high-tone pattern repeating about every 3.5 seconds.

The browser translation of ADL.ASM `voiceoff` omitted `AND AL,0DFh`. Other Worlds calls this routine from its landing-screech timeout with elapsed PIT ticks in AX. When bit 11 of AX is set, the missing mask allows OPL KEY-ON to be set, retriggering a tone that should be stopped. The first affected AX is 0x0800; the cycle repeats every 4096 ticks, approximately 3.515 seconds. This is a translation defect, not anti-piracy or the manual’s stuck-stall warning issue.

Restored the original key-off mask. The historical DOS executable was independently executed for AX 0x0800, 0x0900 and 0xFFFF, confirming register values 0x01, 0x05 and 0xDD respectively.

The new WebAssembly regression failed on the old build at AX 0x0800 and passes after the fix. It covers all 65,536 AX values, idle Other Worlds timers through two 16-bit wraps with and without a sound toggle, and legitimate landing-screech/stall start-stop behavior. The public npm test suite includes it.

The recording’s repeat interval supports this diagnosis; no live Cloudflare deployment or post-deployment player confirmation was performed during the fix.
