#pragma once
#include <stdint.h>
extern "C" {
// Uses the canonical Other Worlds cursor/mesh bank at FE00..FF5A.
// Reset once after cold DS load. begin is original mapkey B-release branch.
void cc_builder_reset();
void cc_builder_startup_options(uint32_t controls);
void cc_builder_begin();
uint32_t cc_builder_active();
uint32_t cc_builder_object();
uint32_t cc_builder_cursor();
uint32_t cc_builder_nomove();
uint32_t cc_builder_begin_frame();
void cc_builder_after_frame();
// Source mbguts continuation:0 frame complete;1 render another mbdrawscene;
// 2 returned to mapkey;3 source error;4 sample original joytd now and call axes.
uint32_t cc_builder_dispatch();
void cc_builder_axes(int32_t centered_x,int32_t centered_y);
void cc_builder_scene_begin(uint32_t raw_frame_ticks);
void cc_builder_near_begin();
uint32_t cc_builder_near_next(); // render returned pointer;0 done;FFFFFFFF bad state
uint32_t cc_builder_scene_finish(); // copytable then dotiles
// Original executable leaf interfaces, useful for retained input/output tests.
void cc_builder_speed_key();
void cc_builder_quality_key();
void cc_builder_set_quality(uint32_t quality);
uint32_t cc_builder_get_quality();
uint32_t cc_builder_valid_type(uint32_t type); // original carry (1 rejected)
uint32_t cc_builder_copy_template(uint32_t target,uint32_t type);
uint32_t cc_builder_move_cursor(int32_t x,int32_t y);
uint32_t cc_builder_step();
uint32_t cc_builder_zoom_object(uint32_t object);
void cc_builder_numbers(); // mbnumbers, both original EGA pages
void cc_builder_map_numbers(); // mapnumbers, both original EGA pages
// Private original mbdefz retained across B entries; initial value -1.
int32_t* cc_builder_private();
}
