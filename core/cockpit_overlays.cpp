#include "cockpit_module.hpp"
#include "state.hpp"
using namespace cc;using namespace cc::state;
namespace f=cockpit_field;
extern "C" {uint8_t* cc_video_memory();void cc_video_load(uint32_t);uint32_t cc_atn2(uint32_t,uint32_t);}
namespace {
unsigned page_offset(unsigned page){return page&1?0:0x7e00;}
void point(unsigned page,int x,int y,unsigned color,unsigned mask){
    // Pixeladdr10 uses a word SHR for x and a wrapped16 EGA byte address.
    unsigned address=uint16_t(y*80+(uint16_t(x)>>3)+page_offset(page));
    auto& pixel=cc_video_memory()[address*8+(unsigned(x)&7)];
    pixel=uint8_t((pixel&(~mask&15))|(color&mask));
}
void line(unsigned page,int x,int y,int x2,int y2,unsigned color,unsigned mask){
    // Literal EGALINE inclusive Bresenham, not KLINE's shallow-row shortcut.
    if(x2<x){int t=x;x=x2;x2=t;t=y;y=y2;y2=t;}
    sw(f::argx1,x);sw(f::argy1,y);sw(f::argx2,x2);sw(f::argy2,y2);wb(f::color,color);
    int dx=x2-x,dy=y2>=y?y2-y:y-y2,sy=y2>=y?1:-1,major=dx>=dy?dx:dy,minor=dx>=dy?dy:dx,error=2*minor-major;
    for(int i=0;i<=major;++i){point(page,x,y,color,mask);bool step=error>=0;if(dy<=dx){++x;if(step)y+=sy;}else{y+=sy;if(step)++x;}error+=2*minor-(step?2*major:0);}
}
void clear_ticks(unsigned page){
    sw(f::argx1,360);sw(f::argx2,430);
    for(int y=212;y<=236;++y)for(int x=360;x<=429;++x)point(page,x,y,0,8);
}
void tick(unsigned page,unsigned kind,int angle){
    int a=w(angle);if(kind)a=w(-a);a=w(a+512);
    if(kind==1){int y=w(a*25/1024+212);line(page,365,y,371,y,8,8);}
    else{int x=w(a*44/1024+385),y=kind==2?214:230;line(page,x,y,x,y+4,8,8);}
}
}
// INST.ASM drawgs:CED3. The caller normally has mapmask15 selected.
extern "C" void cc_cockpit_gunsight_page(uint32_t page){
    page&=1;unsigned win=u(f::front_window);int x=s(win+16),y=s(win+18);
    line(page,x-12,y,x-4,y,7,15);line(page,x-12,y+1,x-4,y+1,7,15);
    line(page,x+13,y,x+5,y,7,15);line(page,x+5,y+1,x+13,y+1,7,15);
    line(page,x,y-8,x,y-3,7,15);line(page,x+1,y-8,x+1,y-3,7,15);
    line(page,x,y+4,x,y+9,7,15);line(page,x+1,y+4,x+1,y+9,7,15);
    cc_video_load(page);
}
extern "C" void cc_cockpit_clear_ticks_page(uint32_t page){clear_ticks(page&1);cc_video_load(page&1);}
extern "C" uint32_t cc_cockpit_tick_page(uint32_t page,uint32_t kind,int32_t angle){
    if(kind>2)return 2;
    tick(page&1,kind,angle);cc_video_load(page&1);return 0;
}
extern "C" void cc_cockpit_controls_page(uint32_t page){
    if(u(u(f::front_window)+6)>199)return;
    page&=1;clear_ticks(page);tick(page,0,s(f::xang));tick(page,1,s(f::yang));tick(page,2,s(f::rdrang));cc_video_load(page);
}
// 3.ASM:7323, original block2A54..2B2B. Returns2 for ATN2's DIV trap.
extern "C" uint32_t cc_cockpit_home_page(uint32_t page){
    for(unsigned i=0;i<12;++i)wb(f::arrowbuf+i,b(f::startcoords+i));
    int x=w(((uint32_t(l(f::startcoords))-uint32_t(l(f::pos)))<<4)>>16);
    int y=w(((uint32_t(l(f::startcoords+4))-uint32_t(l(f::pos+4)))<<4)>>16);
    unsigned distance=uint16_t(uint16_t(absw(x))+uint16_t(absw(y)));
    wb(f::arrowflag,distance>10);
    uint32_t result=cc_atn2(uint16_t(x),uint16_t(y));if(result&65536u)return 2;
    int angle=w(result-u(f::angles));sw(f::homeangle,angle);
    if(b(f::arrowflag)){
        int aim=clamp(-sar(angle,1),-4000,4000),px=aim*310/4000+310,bottom=s(u(f::front_window)+6);
        line(page&1,px,bottom+8,px+2,bottom+2,0,15);line(page&1,px+4,bottom+8,px+2,bottom+2,0,15);
    }
    cc_video_load(page&1);return 0;
}
extern "C" uint32_t cc_cockpit_gunsight_visible(uint32_t key4c,uint32_t key52){return !b(f::eject)||!(key4c||key52);}
