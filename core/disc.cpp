#include "viewport.hpp"
#include <stdint.h>
#include "circles.hpp"
extern "C" uint8_t* cc_framebuffer();
extern "C" uint32_t cc_csqrt(uint32_t);
namespace {
using namespace cc::view;
int s(uint32_t v){v&=65535;return v<32768?int(v):int(v)-65536;}
void span(int di,int left,int right,uint32_t color){
    if(di<window(MinByte)||di>=window(MaxByte))return;
    if(left>right){int t=left;left=right;right=t;}
    if(left<window(Left))left=window(Left);
    if(right>window(Right))right=window(Right);
    auto* frame=cc_framebuffer();
    for(int x=left;x<right;++x)frame[(di/80)*640+x]=uint8_t(color);
}
}
// F3DVEC.ASM drawdisc/smldisc. Solid EGA color masks in the main window.
extern "C" uint32_t cc_draw_disc(uint32_t x,uint32_t y,uint32_t width,uint32_t color){
    color&=15; // EGA set/reset ignores upper bits; discs never stipple.
    const int xc=s(x),yc=s(y),xw=s(width),limited=xw>=1024?1023:xw;
    const uint16_t yw=uint16_t((uint32_t(uint16_t(limited))*45397u)>>16);
    if(s(yc-int(yw/2))>window(Bottom)||s(yc+int(yw/2))<window(Top)||limited<0)return 1;
    if(limited<17){
        if(s(xc-window(Left))<0||s(xc-window(Right))>=0||s(yc-window(Top))<0||s(yc-window(Bottom))>=0)return 1;
        auto* frame=cc_framebuffer();
        const int base=yc*80+xc/8-1;
        for(unsigned row=circle_starts[limited];row<circle_starts[limited+1];++row){
            const uint32_t mask=(uint32_t(circle_rows[row][1])<<8)>>(xc&7);
            for(unsigned byte=0;byte<3;++byte){
                const int address=uint16_t(base+circle_rows[row][0]+int(byte));
                const unsigned bits=(mask>>(16-byte*8))&255;
                for(unsigned bit=0;bit<8;++bit){
                    const unsigned pixel=unsigned(address)*8+bit;
                    if(pixel<640*350 && (bits&(128u>>bit)))frame[pixel]=uint8_t(color);
                }
            }
        }
        return 0;
    }
    if(yc*80<=-65536||yc*80>=65536)return 1;
    const uint32_t ysf=(2936u<<16)/uint16_t(xw);
    const unsigned ysfig=(ysf<<4)>>16;
    if(!ysfig)return 2;
    int lower=s(yc*80),upper=lower;
    span(lower,s(xc-(uint16_t(xw)>>1)),s(xc+((uint16_t(xw+1))>>1)+1),color);
    uint32_t accum=0;
    for(unsigned row=0;row<(unsigned(yw)+1)/2;++row){
        accum+=ysf;
        lower=s(lower+80);if(lower>=window(MaxByte))lower=s(lower-80);
        const unsigned diameter=uint16_t(cc_csqrt(accum>>16)<<5)/ysfig;
        const int left=s(xc-int(diameter>>1)),right=s(xc+int(uint16_t(diameter+1)>>1)+1);
        span(lower,left,right,color);
        upper=s(upper-80);if(upper<0)upper=s(upper+80);
        span(upper,left,right,color);
    }
    return 0;
}

// drawdisc returns the carry of its last CMP on several no-pixel exits.
extern "C" uint32_t cc_disc_carry(uint32_t x,uint32_t y,uint32_t diameter){
    const int width=s(diameter)<1024?s(diameter):1023;
    const int half=s((uint32_t(uint16_t(width))*45397u)>>16)>>1;
    const int top=s(y-half),bottom=s(y+half);
    if(top>window(Bottom))return uint16_t(top)<uint16_t(window(Bottom));
    if(bottom<window(Top))return uint16_t(bottom)<uint16_t(window(Top));
    if(width<0||width>=17)return 0;
    const int a[4]={s(x),s(x),s(y),s(y)},b[4]={window(Left),window(Right),window(Top),window(Bottom)};
    for(unsigned i=0;i<4;++i)if((s(a[i]-b[i])<0)==!(i&1))return uint16_t(a[i])<uint16_t(b[i]);
    return 0;
}
