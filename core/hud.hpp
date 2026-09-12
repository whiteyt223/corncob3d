#pragma once
#include <stdint.h>
extern "C" {
// Original EGA color replacement: AL search color, AH replacement. Writes both
// physical pages and leaves display selection to the host. Status0/2 invalid.
uint32_t cc_hud_button(uint32_t index,uint32_t packed_ax);
uint32_t cc_hud_damage(uint32_t kind);
uint32_t cc_hud_flash_damage(uint32_t kind);
uint32_t cc_hud_damage_indicator_init(uint32_t kind); // previous bar count; UINT32_MAX invalid
void cc_hud_damage_init();
void cc_hud_redraw_buttons();
void cc_hud_flash_alt();
void cc_hud_flash_eject();
void cc_hud_flash_stall(uint32_t original_random_word); // never advances RNG itself
void cc_hud_altitude_warning(); // source4256..42C2; queues low-alt radio message
void cc_hud_complete_button(); // source46D6..46E7
// Draw original DS radio record on physical offset0 / logical page1. No clock
// or modal mutation, except original raster scratch fields and saveadr.
uint32_t cc_radio_draw(uint32_t message_pointer);
// Nonblocking source blackbox. 0 idle/completed;1 modal active;2 invalid record.
// Call queued BEFORE incrementing frmnum. While active only update raw clock and
// original keyboard latches; suspend world/flight. Message always displays page1.
uint32_t cc_radio_begin_queued();
uint32_t cc_radio_open(uint32_t message_pointer,uint32_t clear_queue_on_close);
uint32_t cc_radio_update();
void cc_radio_reset();
// [phase0idle/1delay-down/2release,pointer,saved_vboff,start_tick,display_page,clear_queue]
uint32_t* cc_radio_state();
}
