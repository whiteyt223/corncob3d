#pragma once
#include <stdint.h>
namespace cc::view {
// F3DEQU window word order. Inline state keeps primitive-only native builds
// self-contained, including callers that do not link the optional wrappers.
enum {Left,Top,Right,Bottom,Width,Height,Zx,Zy,Cx,Cy,MinByte,MaxByte};
inline constexpr uint16_t presets[3][12]={
    {8,4,631,199,623,195,300,200,319,101,321,15920},
    {8,4,631,345,623,341,300,200,319,174,321,27600},
    {168,254,471,344,303,90,150,100,320,299,20341,27520}
};
inline uint16_t active[12]={8,4,631,199,623,195,300,200,319,101,321,15920};
inline int window(unsigned word){return int16_t(active[word]);}
inline void set(const uint16_t* words){for(unsigned i=0;i<12;++i)active[i]=words[i];}
}
extern "C" {
// IDs: 0 cockpit forward, 1 full forward, 2 tail. Primitives use the active
// words; original no-configuration callers retain the cockpit forward window.
const uint16_t* cc_viewport_words(uint32_t id);
uint16_t* cc_viewport_active();
uint32_t cc_viewport_select(uint32_t id);
void cc_viewport_set(const uint16_t* words);
void cc_views_configure(uint32_t full_screen,uint32_t rear_enabled);
// Custom source ptrgwinf words, retained across the primitive wrappers.
void cc_views_custom_front(const uint16_t* words);
void cc_views_fill(uint32_t enabled);
void cc_views_flags(uint32_t horizon,uint32_t diving,uint32_t sky,uint32_t ground);
// Camera-coordinate input, 42 vectors to retain polyhell's source extension.
uint16_t* cc_views_input();
// front status, rear status (FFFFFFFF if disabled), objfail delta, linefail,
// final projected diameter, final polygon count. Primitive status2 is IDIV.
uint32_t* cc_views_result();
uint32_t cc_view_point(uint32_t color);
uint32_t cc_view_disc(uint32_t diameter,uint32_t color);
// Status3: unclipped filled horizon invokes original HLINE with invalid SI;
// it needs the surrounding DS scratch memory and is outside this safe API.
uint32_t cc_view_line(uint32_t color,uint32_t rdf);
uint32_t cc_view_polygon(uint32_t count,uint32_t color,uint32_t rdf);
uint32_t cc_render_views_point(uint32_t color);
uint32_t cc_render_views_disc(uint32_t diameter,uint32_t color);
uint32_t cc_render_views_wire(uint32_t color,uint32_t rdf);
uint32_t cc_render_views_polygon(uint32_t count,uint32_t color,uint32_t rdf);
uint32_t cc_resolve_draw_color(uint32_t word);
// epage modifies full indexed EGA storage, then loads the logical page into
// cc_framebuffer. Store pending framebuffer edits before calling either API.
uint32_t cc_epage_page(uint32_t page,uint32_t color,uint32_t rear_enabled,uint32_t jump_flag,uint32_t jump_value);
uint32_t cc_epage_state_page(uint32_t page);
}
