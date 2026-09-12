#include "viewport.hpp"
#include <stdint.h>

// F3DVEC.ASM drawpoly/ladvance/radvance and rawhlinewin span boundaries.
// Solid palette colors only. EGA write masks and stipple/random colors remain
// separate. divrs_dw rounding recovered from shareware 3.EXE at file E913h.
namespace {
using namespace cc::view;
int32_t s16(uint32_t v) { v&=65535;return v<32768?int32_t(v):int32_t(v)-65536; }
int32_t wrap(int32_t v) { return s16(uint32_t(v)); }
uint32_t rounded(int32_t numerator,int32_t divisor) {
    const bool negative=(numerator<0)!=(divisor<0);
    const uint32_t a=numerator<0 ? 0u-uint32_t(numerator):uint32_t(numerator);
    const uint32_t b=divisor<0 ? uint32_t(-divisor):uint32_t(divisor);
    const uint32_t q=a/b+((a%b)*2>=b?1:0);
    return negative?0u-q:q;
}
uint16_t vertices[40];
uint8_t framebuffer[640*350];
uint16_t spans[350*3];
uint32_t span_count;
uint32_t random_seed=0x002d007b;
uint32_t random_word(){random_seed=uint32_t(uint64_t(random_seed)*0x278dde6du);return random_seed>>16;}
struct Edge {
    int index,x1,y1,x2,y2,step;
    uint32_t fixed,slope;
    bool advance(int count) {
        int level=1;
        for(;;) {
            x1=x2;y1=y2;fixed=(uint32_t(uint16_t(x1))<<16)|(fixed&65535u);
            index=(index+step+count)%count;
            x2=s16(vertices[index*2]);y2=s16(vertices[index*2+1]);
            int dy=wrap(y2-y1);
            if(dy<0)return false;
            if(dy) {
                int dx=wrap(x2-x1);
                // CWD followed by XCHG AX,DX leaves FFFFh in the low
                // word for a negative delta; it is not simply dx*65536.
                const int32_t numerator=dx*65536+(dx<0?65535:0);
                slope=rounded(numerator,dy);
                fixed=(uint32_t(uint16_t(x1))<<16)|32767u;
                return true;
            }
            if(--level<0)return false;
        }
    }
    int x() const {return s16(fixed>>16);}
    void jump(int delta) {fixed+=slope*uint32_t(delta);}
};
void emit(int y,int x1,int x2,uint32_t color,uint32_t mask=255) {
    // rawhlinewin checks DI against gfmnb=4*80+1, excluding row four.
    if(y*80<window(MinByte)||y*80>=window(MaxByte))return;
    if(x1>x2){int temp=x1;x1=x2;x2=temp;}
    if(x2<window(Left)||x1>=window(Right))return;
    if(x1<window(Left))x1=window(Left);
    if(x2>window(Right))x2=window(Right);
    if(x1>=x2)return;
    if(span_count<350){spans[span_count*3]=uint16_t(y);spans[span_count*3+1]=uint16_t(x1);spans[span_count*3+2]=uint16_t(x2);++span_count;}
    const int width=x2-x1,phase=x1&7,start=y*640+(x1&~7);
    auto paint=[&](int byte,uint32_t bits,const uint8_t* latches){
        for(unsigned bit=0;bit<8;++bit){int at=start+byte*8+int(bit);framebuffer[at]=(bits&(128u>>bit))?uint8_t(color):(latches?latches[bit]:framebuffer[at]);}
    };
    if(width<=16){
        const uint32_t effective=width>8&&!phase?255:mask;
        for(int byte=0;byte<3;++byte){uint32_t bits=0;for(int bit=0;bit<8;++bit){int x=(x1&~7)+byte*8+bit;if(x>=x1&&x<x2)bits|=128u>>bit;}if(bits)paint(byte,bits&effective,nullptr);}
    }else{
        uint8_t latches[8];for(unsigned bit=0;bit<8;++bit)latches[bit]=framebuffer[start+bit];
        paint(0,mask>>phase,latches);int remaining=width-8+phase;
        for(int byte=0;byte<remaining/8;++byte)paint(1+byte,mask,latches);
        paint(1+remaining/8,(mask<<(8-(remaining&7)))&255,nullptr);
    }
}
}
extern "C" uint16_t* cc_raster_vertices(){return vertices;}
extern "C" uint8_t* cc_framebuffer(){return framebuffer;}
extern "C" uint16_t* cc_raster_spans(){return spans;}
extern "C" uint32_t cc_raster_span_count(){return span_count;}
extern "C" void cc_reset_spans(){span_count=0;}
extern "C" void cc_raster_set_seed(uint32_t value){random_seed=value;}
extern "C" uint32_t cc_raster_get_seed(){return random_seed;}
extern "C" void cc_clear(uint32_t color){span_count=0;for(auto &p:framebuffer)p=uint8_t(color);}
extern "C" uint32_t cc_draw_polygon(uint32_t count,uint32_t color) {
    span_count=0;
    if(count==0||count>20||color>255)return 1;
    int minimum=32767,maximum=-32767,index=0;
    for(unsigned i=0;i<count;++i){int y=s16(vertices[i*2+1]);if(y<=minimum){minimum=y;index=int(i);}if(y>=maximum)maximum=y;}
    Edge left{index,0,0,s16(vertices[index*2]),minimum,1,0,0};
    Edge right{index,0,0,s16(vertices[index*2]),minimum,-1,0,0};
    if(!left.advance(int(count))||!right.advance(int(count)))return 0;
    const bool stipple=color>15;
    if(stipple)color=random_word()&0x55&15;
    int y=minimum,delta=0;
    // The original normally terminates at the maximum vertex or window edge.
    // Bound malformed user geometry without an unbounded browser loop.
    for(unsigned budget=0;budget<70000;++budget){
        if(left.y2<=y){if(!left.advance(int(count))||maximum<=left.y1)return 0;}
        if(right.y2<=y){if(!right.advance(int(count)))return 0;}
        if(y<window(Top)){
            if(left.y2>right.y2 && right.y2<=window(Top)){
                if(!right.advance(int(count))||maximum<=right.y1)return 0;
                delta=wrap(right.y1-y);y=right.y1;if(delta<0)return 0;left.jump(delta);
            }else if(left.y2<=right.y2 && left.y2<=window(Top)){
                if(!left.advance(int(count))||maximum<=left.y1)return 0;
                delta=wrap(left.y1-y);y=left.y1;if(delta<0)return 0;right.jump(delta);
            }else{
                delta=wrap(window(Top)-y);
                if(delta>=0){y=window(Top);left.jump(delta);right.jump(delta);}
            }
            // skipadone jumps directly to noabovedone, before edge rechecking.
        }else if(y>=window(Bottom))return 0;
        const uint32_t mask=stipple&&y>=window(Top)?random_word()&0x55:255;
        emit(y,left.x(),right.x(),color,mask);
        y=wrap(y+1);left.jump(1);right.jump(1);
    }
    return 2;
}
