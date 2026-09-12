#pragma once
#include <stdint.h>
extern "C" {
// Source remote inset: caller selects DS gwinl and clears it, then this sets
// the near-mesh scene state. Returns original octant0..7; no distance/LOD test.
uint32_t cc_camera_remote_prepare();
// erasewinf into the pending framebuffer using DS ptrgwinf window geometry.
void cc_camera_erase_front();
// Pending framebuffer edits must be stored before switching. Updates source
// border/shake/audio/DS page state; returns next draw page (0 or1).
uint32_t cc_camera_flip_page();
// CRTC/BDA start address, EGA bytes; includes the source screen-shake offset.
uint32_t cc_camera_display_address();
// Present source CRTC offset after all pending video/HUD writes are stored.
void cc_camera_present();
// R-release handler:0 untouched,1 enabled,2 disabled. Disabling clears both
// video interiors and calls flipage twice. No pending framebuffer is stored.
uint32_t cc_camera_rear_input();
}
