#pragma once
#include <stdint.h>
// Existing integration dependencies: state.hpp, fixed.hpp, runtime.hpp,
// cc_video_memory/cc_video_load, cc_atn2 and cc_matvmul.
// Logical page0 is EGA vboff0x7e00; logical page1 is vboff0.
namespace cockpit_field {
constexpr unsigned homeangle=0x0282,arrowflag=0x0284,arrowbuf=0x0297,startcoords=0x02a9;
constexpr unsigned windtick=0x0add,eject=0x0afc,pos=0x0b05,angles=0x0b11;
constexpr unsigned propvec=0x0e78,remote=0x0f54,frozen=0x0f7b;
constexpr unsigned crshvel=0x19c0,engdam=0x1a0b;
constexpr unsigned argx1=0x1b83,argy1=0x1b85,argx2=0x1b87,argy2=0x1b89,color=0x1b8b;
constexpr unsigned xang=0x1b92,yang=0x1b94,rdrang=0x1b96,front_window=0x1c3e;
constexpr unsigned xvec=0x1d06,speed=0x1d9c,rpm=0x1de2,rpmx=0x1de4,currpmx=0x1de6;
constexpr unsigned thrtmult=0x1e24,temperature=0x1e5a,oil=0x1e5c,norot=0x1ff0;
constexpr unsigned prop_object=0x5bfa,windflag=0xf5b2,rng_seed=0xf722;
// CS:0043 oticks is supplied by existing cc::raw_ticks(), not another DS word.
}
extern "C" {
void cc_cockpit_gunsight_page(uint32_t page);
void cc_cockpit_clear_ticks_page(uint32_t page);
// kind0=aileron,1=elevator,2=rudder. Angles are original signed16 words.
uint32_t cc_cockpit_tick_page(uint32_t page,uint32_t kind,int32_t angle);
void cc_cockpit_controls_page(uint32_t page);
// Calculates/stores homeangle and arrowflag, and paints the original black V.
uint32_t cc_cockpit_home_page(uint32_t page);
uint32_t cc_cockpit_gunsight_visible(uint32_t key4c,uint32_t key52);
// Update return:0 normal,2 original DIV failure. Oil updates even when frozen.
uint32_t cc_cockpit_oil_update();
uint32_t cc_cockpit_temperature_update(uint32_t incoming_ax);
uint32_t cc_cockpit_engine_update();
uint32_t cc_cockpit_temperature_ax(); // diagnostic: exact residual AX of last engine update
void cc_cockpit_prop_update();
uint32_t cc_cockpit_prop_visible();
}
