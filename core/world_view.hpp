#pragma once
#include <stdint.h>
extern "C" {
// Same world_input/output/local interfaces as cc_prepare_object. Uses live DS
// observer/matrices; input0..5 are ignored. Pass source object type word+24.
// Source zero-type/range/map-far skips return1; successful preparation returns0.
uint32_t cc_prepare_object_view(uint32_t object_flags);
}
