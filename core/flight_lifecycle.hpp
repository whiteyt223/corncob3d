#pragma once
#include <stdint.h>

// Additive original-aircraft/pilot lifecycle module. All functions operate on
// cc_flight_state(), the same packed DS image used by the existing flight core.
extern "C" {
// Optional synchronous world hook. kind1=funeral5d(obj, destroyed); return1 if
// the original persistent-object transition was handled in full. Otherwise the
// module queues a persistence event and removes the old live obj5d itself.
typedef uint32_t (*cc_lifecycle_hook)(uint32_t kind,uint32_t object,uint32_t detail);
void cc_lifecycle_set_hook(cc_lifecycle_hook hook);
void cc_lifecycle_reset(); // host/session state only; does not replace DS
uint32_t cc_lifecycle_terminal();
// Events: count at[0], stride22 u32 from[1]: kind,source,detail,19 packed words
// containing the 74-byte source record. kind1=persist wreck;2=fullsize;3=cockpit;
// 4=fatal mission exit. Consume/reset explicitly; overflow reports source error164.
uint32_t* cc_lifecycle_events();
void cc_lifecycle_clear_events();

uint32_t cc_aircraft_damage(uint32_t count);
uint32_t cc_aircraft_damage_component(uint32_t kind); // 0 engine,1 wing,2 stab,3 yaw
void cc_aircraft_clear_damage();
void cc_aircraft_damage_bar(uint32_t kind); // exact indicator DS state, 0 engine through 3 yaw
void cc_aircraft_fullsize();
int32_t cc_aircraft_damage_bias(int32_t damage,int32_t speed);
void cc_aircraft_stability_noise();
void cc_aircraft_crash_prelude();
void cc_aircraft_crash_damping();
void cc_aircraft_qcrashland();
void cc_aircraft_resetplane();
uint32_t cc_aircraft_gear(uint32_t brake); // 0 normal state transition,2 arithmetic
uint32_t cc_aircraft_resolve_crash(); // call late, after throttle;1 terminal death
void cc_aircraft_home();
void cc_aircraft_common_status();
void cc_aircraft_destroy_abandoned();
void cc_aircraft_eject(); // E-release action, late in frame; no invented impulse
void cc_aircraft_pull_chute(); // Space-release action
uint32_t cc_aircraft_try_reenter();
uint32_t cc_aircraft_board(uint32_t object); // static getinplane:0 success,1 refused,2 waiting
uint32_t cc_aircraft_board_pending(); // held joystick-key5 original wait; retry board on release

// Early/late matrix split and pilot phases used by flight_extended.cpp.
uint32_t cc_aircraft_angles_offset();
void cc_aircraft_refresh_all();
void cc_pilot_prepare_matrices();
void cc_pilot_look(int32_t centered_x,int32_t centered_y);
void cc_pilot_move();
void cc_pilot_damp();
void cc_pilot_swing(int32_t second_x,int32_t second_y);
void cc_pilot_walk(uint32_t forward,uint32_t backward);
void cc_pilot_step(uint32_t bypass_cooldown);
void cc_aircraft_move_abandoned();

// Original keyboard data at DS02C7/02D7. Timestamps are raw original ticks;
// cc_runtime_state()[0] supplies the current raw clock during a frame.
void cc_lifecycle_key_event(uint32_t scan,uint32_t raw_tick);
uint32_t cc_lifecycle_key_down(uint32_t scan);
uint32_t cc_lifecycle_readkey(uint32_t scan); // held low16,elapsed high16
uint32_t cc_lifecycle_scalekey(uint32_t scan,uint32_t scale);
uint32_t cc_lifecycle_arrows(); // signed centered X low16,Y high16; keyboard path
}
