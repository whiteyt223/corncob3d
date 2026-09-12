#pragma once
#include <stdint.h>
extern "C" {
// Call early before frmnum/render after queued radio messages.0 waiting/idle,
// 1 map entered,2 arithmetic. Honors original M-release/radio/tutorial gates.
uint32_t cc_map_request();
void cc_map_reset(); // private host phase only; leave DS untouched
uint32_t cc_map_active();
uint32_t cc_map_begin(); // original D895..D8FF, for direct/fullscreen entry
void cc_map_end(); // original D994..D9A9, restores saved observer and desire
void cc_map_frame_begin(uint32_t raw_frame_ticks); // D8FF..D939
// Original joytd result after centering. The host obtains original keyboard
// values with its existing joytd adapter, exactly once per map iteration.
uint32_t cc_map_controls(int32_t centered_x,int32_t centered_y);
uint32_t cc_map_after_controls(); // exit gate+cleanup:1 exited,0 continue
// Original drawmap82D0. Iterate all64 packed tiles, retaining DS tile fields.
// Render each returned memobjbuf pointer before requesting next.0 complete,
// UINT32_MAX invalid host state. No simulation/activation/collision callbacks.
void cc_map_draw_begin();
uint32_t cc_map_draw_next();
uint32_t cc_map_actor();
uint32_t cc_map_marker(); // original drawyahD86A: color+visibility gate
void cc_map_restore_observer(); // restoreoxD9AA
uint32_t cc_map_bump_angles(int32_t pitch,int32_t yaw); // D9FF
uint32_t cc_map_zoom(); // D6E2, keysPageDown/PageUp/S
}
