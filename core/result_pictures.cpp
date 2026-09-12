#include "result_pictures.hpp"
namespace {
uint16_t word(const uint8_t* p) { return uint16_t(p[0] | (uint16_t(p[1]) << 8)); }
void putword(uint8_t* p, uint16_t v) { p[0]=uint8_t(v);p[1]=uint8_t(v>>8); }
void empty(uint8_t* out, uint8_t grflag) {
    out[0]=0;out[1]=grflag;out[2]=0;out[3]=0;
    out[4]=255;out[5]=255;out[6]=255;out[7]=0;
}
struct Plan {
    uint8_t* out;
    void show(unsigned picture, bool qgetch) {
        if(qgetch && out[1])out[3]|=uint8_t(1u<<out[0]);
        out[4+out[0]++]=uint8_t(picture);out[1]=255;
    }
    unsigned done() { if(!out[1])out[2]=1;return out[0]; }
};
}
extern "C" uint32_t cc_result_picture_sequence(uint32_t status, uint32_t death,
    uint32_t friendly_destroyed, uint32_t grflag, uint8_t* out) {
    const uint16_t ax=uint16_t(status),dc=uint16_t(death);
    empty(out,uint8_t(grflag));Plan p{out};
    if(ax&0x4000)return 0;
    if(ax&0x400){p.show(14,false);return p.done();}
    if(!(ax&0x800) && !(ax&8) && (ax&2)) {
        p.show((ax&0x20)?7:4,bool(ax&0x20));return p.done();
    }
    // Capture suppresses the initial crash picture except when the original
    // killed-by-object bit bypasses the captured/dead tests.
    if((ax&0x800) || !(ax&8)) {
        if(ax&0x40) {if(ax&(0x100|0x10))p.show(2,false);}
        else if(ax&0x200)p.show(3,false);
    }
    if(ax&8){p.show(8,true);return p.done();}
    if(ax&0x100)p.show(1,true);
    else {
        if(ax&0x800){p.show(dc==0?7:dc==22?10:dc==24?11:dc==32?12:13,true);return p.done();}
        if((ax&0x14)==0x14 && !(ax&0x240)) {if(!(ax&1))p.show(0,true);}
        else if(!(ax&0x10)) {if(!(ax&1))p.show(9,true);}
        else if(!(ax&0x220) && !(ax&1))p.show(5,true);
    }
    if(uint16_t(friendly_destroyed))p.show(6,true);
    return p.done();
}
extern "C" uint32_t cc_result_picture_plan(uint8_t* ds, uint32_t cecode, uint8_t* out) {
    if(uint8_t(cecode)){empty(out,ds[0xf52a]);return 0;}
    const unsigned count=cc_result_picture_sequence(word(ds+0x1e4),word(ds+0x1e6),
        word(ds+0x22e),ds[0xf52a],out);
    ds[0xf52a]=out[1];if(count)putword(ds+0xf52d,out[3+count]);
    return count;
}
