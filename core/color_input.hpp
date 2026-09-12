#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void cc_color_input_reset();
// CS404E..406E/takedrugs207B. Return0 complete,1 waiting key1 release,
// 2 waiting key2 release,3 invalid time arithmetic. While waiting freeze game
// advancement and call again after keyboard state changes.
uint32_t cc_color_input();
// 1 if this call ran distcolors: apply DS1EAE's48 RGB bytes (stride4) to DAC.
uint32_t cc_color_input_palette_dirty();
#ifdef __cplusplus
}
#endif
