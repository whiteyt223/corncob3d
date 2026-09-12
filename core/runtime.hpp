#pragma once
#include <stdint.h>
extern "C" uint32_t* cc_runtime_state();
namespace cc {
inline uint16_t raw_ticks(){return uint16_t(cc_runtime_state()[0]);}
inline void source_error(unsigned code){cc_runtime_state()[1]=uint8_t(code);}
}
