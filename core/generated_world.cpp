#include "generated_world.hpp"
#include "state.hpp"
using namespace cc;using namespace cc::state;
namespace {
constexpr unsigned kbuf=0x1a1e,coords=0xe7e,quality=0xec30;
uint32_t result[3];bool failed;
void copy(unsigned from,unsigned to,unsigned count){for(unsigned i=0;i<count;++i)wb(to+i,b(from+i));}
unsigned allocate(unsigned prototype,unsigned source){
    sw(0x8fa2,source);sw(0xec28,prototype);++result[0];
    for(unsigned p=0x8fa6;p<0xec26;p+=74)if(!u(p+24)&&(b(p+18)&128)){
        sw(0x8fa4,p);copy(prototype,p,74);sw(0xec28,p);++result[1];failed=false;return p;
    }
    ++result[2];failed=true;return 0;
}
void mark(unsigned p){wb(p+18,0);sw(p+24,u(p+24)|0x8000|((u(quality)&7)<<12));}
void perm(unsigned prototype,unsigned source=kbuf){unsigned p=allocate(prototype,source);if(p){mark(p);copy(source,p,18);}}
void xyz(int32_t x,int32_t y,int32_t z,int yaw=0,int pitch=0,int roll=0){sl(kbuf,x);sl(kbuf+4,y);sl(kbuf+8,z);sw(kbuf+12,yaw);sw(kbuf+14,pitch);sw(kbuf+16,roll);}
int32_t grand(){unsigned hi=uint16_t(random_bound(6)-3),lo=random_word();return d((uint32_t(hi)<<16)|lo);}
void random_coords(int z){sl(coords,grand());sl(coords+4,grand());sl(coords+8,z);}
void choose_quality(unsigned p){sw(quality,s(p)<0?random_word()&7:u(p));}
void site(unsigned kind){
    unsigned p=allocate(kind==0?0x85f2:kind==1?0x82c4:0x8230,coords);if(!p)return;
    if(kind==0)sw(p+52,24000);
    else if(kind==1){unsigned twice=uint16_t(u(0xf0f2)*2);sw(p+52,twice+(twice>>1));}
    else{
        unsigned row=0xed31+uint16_t(uint8_t(u(0xed2f)*2)*((u(p+24)>>12)&7));
        sw(0xec32,row);wb(0x72ee,u(row));sw(0xec34,u(row+4));sw(p+52,u(0xec34));
    }
    mark(p);for(unsigned a=0;a<3;++a){sw(p+46+a*2,0);sw(p+12+a*2,0);}if(kind==0)sw(p+14,-16384);
    copy(coords,p,12);if(kind==0)sw(0xf0de,u(0xf0de)+1);
}
}
extern "C" uint32_t* cc_generated_world_result(){return result;}
extern "C" uint32_t cc_initperms(){
    for(unsigned& v:result)v=0;
    failed=false;sw(0x2dce,u(0x2dce)+1);
    // Original count74 STOSW clears148 bytes, not a74-byte object.
    for(unsigned i=0;i<148;++i)wb(kbuf+i,0);
    xyz(-589825,-8000,0);sw(quality,0);perm(0x7f4c);addl(kbuf,-32768);
    for(unsigned i=0;i<10;++i){sw(quality,0);perm(0x7fe0);addl(kbuf,-32768);}
    sw(quality,0);perm(0x7f96);sl(kbuf,d(int64_t(l(0x2a9))-12000));sl(kbuf+4,0);sw(quality,7);perm(0x7fe0);
    if(!b(0xae8)){
        for(unsigned i=0;i<15;++i){random_coords(1550);sw(quality,0);perm(0x8764,coords);}
        for(unsigned n=u(0xe70);n;--n){choose_quality(0xe76);random_coords(800);site(0);}
        copy(0xb05,coords,12);sw(coords+6,u(coords+6)-5);sw(coords+2,u(coords+2)+2);
        sw(0xf0ea,u(0xe6e));for(unsigned n=u(0xe6e);n;--n){choose_quality(0xe74);random_coords(800);site(1);}
        sw(0xf113,u(0xe6c));for(unsigned n=u(0xe6c);n;--n){choose_quality(0xe72);random_coords(800);site(2);}
        // Preserved3.42 has quality0 walls and the saucey HQ/base branch.
        sw(quality,0);xyz(32766,0,16383);perm(0x830e);
        sw(quality,0);xyz(-32766,0,16383);perm(0x830e);
        xyz(0,0,32766);perm(0x83a2);xyz(0,131072,32766);perm(0x8358);
        sw(quality,1);xyz(0,0,11200);perm(0x7dda);
        choose_quality(0xe72);xyz(0,0,163072);perm(0x8c98);
    }
    sw(quality,0);xyz(-712896,8000,u(0xf5c),-8192,-1500);perm(0x7e6e);
    xyz(-589825,16000,3600);perm(0x7f02);perm(0x7eb8);
    xyz(-589825,22000,0);perm(0x8108);xyz(-720897,14000,0);perm(0x802a);
    xyz(-720897,16000,16000);perm(0x8074);
    xyz(0,0,0);perm(0x87ae);perm(0x87ae);xyz(-2097152,-2097152,0);perm(0x87ae);
    xyz(0,-2097152,0);perm(0x87ae);xyz(-2097152,0,0);perm(0x87ae);
    return failed?1:0;
}
