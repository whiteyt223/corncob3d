#pragma once
#include <stdint.h>
extern "C" {
// Original CS43C5..454C, AFTER cc_weapon_input and BEFORE remote-view controls.
// Argument is CS0037 legacy_ticks, incremented once per64 raw PIT interrupts.
// Returns0 completed,2 original unsigned-DIV overflow/zero domain.
uint32_t cc_weapon_late_input(uint32_t legacy_ticks);
// Original strtbomb ADEB and strtbomb1 AD8E. Return carry0 success/1 full pool.
uint32_t cc_weapon_drop_bomb();
uint32_t cc_weapon_bomb_from(uint32_t source);
// Host loads the original640x350 indexed cockpit ONCE; it is retained unchanged.
uint8_t* cc_weapon_cockpit_image();
void cc_weapon_cockpit_ready(uint32_t ready);
// Exact interiorview1E40; source error gate, DS window fields, image reload only.
void cc_weapon_interior_view();
// Exact restoreview E8A. Reloads both physical pages synchronously before HUD.
void cc_weapon_restore_view();
// Monotonic image reload count, useful to invalidate host instrument page caches.
uint32_t cc_weapon_restore_sequence();
}
