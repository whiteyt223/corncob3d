#pragma once
#include <stdint.h>
// Original 3.42 timed callbacks, including rel3d z0 coordinate compression. Uses cc_flight_state().
// Status: 0 handled; 1 unsupported callback/branch; 2 invalid numeric domain.
// Callback status 0 includes original allocation failure and population caps.
// Type2/28 boarding forwards the lifecycle result; status2 with a nonzero
// cc_aircraft_board_pending() is the source joystick release wait.
extern "C" uint32_t cc_enemy_timed(uint32_t object);
extern "C" uint32_t cc_enemy_hit(uint32_t object,uint32_t other);
// Direct shell-opening entry; cc_enemy_hit dispatches here for type49.
extern "C" uint32_t cc_enemy_shell_hit(uint32_t object);
// Observer helper: 0 near, 1 range failure (stale vector retained), 2 invalid.
// force_observer=0 selects abandoned aircraft while ejected; 1 always observer.
extern "C" uint32_t cc_enemy_observer_vector(uint32_t object,uint32_t force_observer);
extern "C" uint32_t cc_enemy_attract(uint32_t object);
extern "C" uint32_t cc_enemy_initnumbers();
extern "C" uint32_t cc_enemy_change_walls();
// Last radar event [serial, object, radius, angle, color]. Consume immediately
// after each timed callback if serial changed. Original radardot painting is
// delegated to the renderer, with the exact pre-paint arguments retained.
extern "C" uint32_t* cc_enemy_radar_event();

// Last DAC update [serial, (palette index, red, green, blue) x16].
// RGB values are original 6-bit DAC values; DS rescolors is also updated.
extern "C" uint32_t* cc_enemy_palette_event();

// disteval including source z0:0 near/medium,1 far,2 invalid.
extern "C" uint32_t cc_enemy_distance_object(uint32_t object);
// Original late V-release branch, consumes DS026c=AF and applies source guards.
extern "C" uint32_t cc_enemy_rescue_request();

extern "C" uint32_t cc_enemy_pilot_distance(uint32_t object);
