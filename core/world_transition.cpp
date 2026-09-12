#include "edition.hpp"
#include "world_transition.hpp"
#include "state.hpp"
#include "universe_fields.hpp"
#include "runtime.hpp"
using namespace cc;using namespace cc::state;namespace f=universe_field;
extern "C" {
uint8_t* cc_universe_memory();
uint32_t cc_farm_tile(int32_t,int32_t);uint32_t cc_bring_tile(int32_t,int32_t);
uint32_t cc_encode_object(uint32_t,int32_t,int32_t,uint32_t);
int32_t cc_ssin(uint32_t);int32_t cc_scos(uint32_t);
void cc_angles_matrix(const uint16_t*,uint16_t*,uint32_t);
void cc_matvmul(const uint16_t*,const uint16_t*,uint16_t*);
}
namespace {
uint32_t events[1+32*3];
void event(unsigned kind,int x=0,int y=0){if(events[0]<32){unsigned at=1+events[0]++*3;events[at]=kind;events[at+1]=uint16_t(x);events[at+2]=uint16_t(y);}}
bool valid(int x,int y){return x>=0&&x<8&&y>=0&&y<8;}
void updatect(){sw(f::ctx,w(s(0xb07)-s(f::universex)+16)/32);sw(f::cty,w(s(0xb0b)-s(f::universey)+16)/32);}
void changethem(){for(unsigned i=0;i<4;++i){sw(f::ctiles+i*4,s(f::ctx)+s(f::ctileoffs+i*4));sw(f::ctiles+i*4+2,s(f::cty)+s(f::ctileoffs+i*4+2));}}
bool select(unsigned i){int x=s(f::ctiles+i*4);sw(f::curtilex,x);if(x<0||x>=8)return false;int y=s(f::ctiles+i*4+2);sw(f::curtiley,y);return valid(x,y);}
bool in_tile(unsigned p){int dx=w(s(p+2)-w(s(f::curtilex)*32+s(f::universex))),dy=w(s(p+6)-w(s(f::curtiley)*32+s(f::universey)));return dx>=0&&dx<32&&dy>=0&&dy<32;}
unsigned tile_index(){return unsigned(s(f::curtiley)*8+s(f::curtilex));}
uint32_t farm(int x,int y){unsigned prior=u(f::cache_exptr);uint32_t r=cc_farm_tile(x,y);sw(f::cache_exptr,prior);return r;}
uint32_t bring(int x,int y){unsigned prior=u(f::cache_exptr);uint32_t r=cc_bring_tile(x,y);sw(f::cache_exptr,prior);return r;}
uint32_t flush_tile(){
    const int x=s(f::curtilex),y=s(f::curtiley);if(!valid(x,y))return 2;
    event(2,x,y);unsigned table=u(f::tblist),count=u(table);sw(f::tbstrt,table);
    if(table+4+count*2>65536)return 2;
    for(unsigned i=0;i<count;++i){
        unsigned object=u(table+4+i*2);if(!u(object+24))continue;
        if(u(f::extendedtypetbl+u(object+72)*2)&1)continue;
        if(u(f::tobjcount)>=149)continue;
        sw(f::tobjcount,u(f::tobjcount)+1); // Original increments BEFORE qintile.
        if(!in_tile(object))continue;
        unsigned ptr=u(f::tileptr);if(ptr%23||ptr/23>=150)return 2;
        unsigned next=0xff90+tile_index()*3456+ptr+34;if(next+1>=0x45f90)return 2;
        cc_universe_memory()[next]=0;cc_universe_memory()[next+1]=0;
        // encodeobj failure is ignored by flushtile: the record is still retired.
        cc_encode_object(object,x,y,ptr/23);sw(f::tileptr,ptr+23);sw(object+24,0);if(!cc_edition_is_deluxe())wb(object+18,255);
    }
    return 0;
}
uint32_t flush_tiles(){
    updatect();changethem();
    for(unsigned i=0;i<4;++i)if(select(i)){
        int x=s(f::curtilex),y=s(f::curtiley);event(1,x,y);if(farm(x,y)==2)return 2;
        if(flush_tile())return 2;
        wb(f::ctstatus+tile_index(),0);
    }
    sw(u(f::tblist),0);return 0;
}
uint32_t dotiles(){
    updatect();
    for(unsigned i=0;i<4;++i)if(select(i)){
        int x=s(f::curtilex),y=s(f::curtiley);
        if((uint16_t(s(f::ctx)-x)&0xfffe)||(uint16_t(s(f::cty)-y)&0xfffe)){
            event(1,x,y);if(farm(x,y)==2)return 2;wb(f::ctstatus+tile_index(),0);
        }
    }
    changethem();
    for(unsigned i=0;i<4;++i)if(select(i)){
        unsigned status=f::ctstatus+tile_index();if(!b(status)){
            int x=s(f::curtilex),y=s(f::curtiley);event(6,x,y);if(bring(x,y)==2)return 2;wb(status,255);
        }
    }
    return 0;
}
void empty_drawnear(){
    // flushtiles already zeroed the only active table. Exact drawnear then
    // writes these two fields and runs no timed callback, collision or renderer.
    event(3);sw(f::objcount,0);sw(f::tbstrt,u(f::tblist));
}
void mission_matrices(){
    uint16_t angles[3],forward[9],inverse[9];for(unsigned i=0;i<3;++i){angles[i]=u(0xb11+i*2);sw(0x20ae +i*4,cc_ssin(angles[i]));sw(0x20b0+i*4,cc_scos(angles[i]));}
    cc_angles_matrix(angles,forward,0);cc_angles_matrix(angles,inverse,1);
    // ncalcmat invokes negsin and leaves the shared sine words negated.
    for(unsigned i=0;i<3;++i)sw(0x20ae +i*4,-s(0x20ae +i*4));
    for(unsigned i=0;i<9;++i){sw(0x1fde +i*2,forward[i]);sw(0x1d36+i*2,forward[i]);sw(0x1ff0+i*2,inverse[i]);sw(0x1d48+i*2,inverse[i]);}
    const uint16_t v[3]={5000,0,0};uint16_t r[3];cc_matvmul(inverse,v,r);
    for(unsigned i=0;i<3;++i){sw(0x1d9c+i*2,v[i]);sw(0x1dc2+i*2,r[i]);sw(0x1dc8+i*2,int(v[i])>>4);}
}
}
extern "C" uint32_t* cc_transition_events(){return events;}
extern "C" uint32_t cc_flush_tile(){events[0]=0;return flush_tile();}
extern "C" uint32_t cc_flush_tiles(){events[0]=0;return flush_tiles();}
extern "C" uint32_t cc_donewmiss(){
    events[0]=0;if(!b(f::newmissflag))return 0;
    if(u(0x1c7)>=16){wb(f::newmissflag,0);sw(0xf7f,0x1249);return 3;}
    if(flush_tiles())return 2;
    empty_drawnear();event(4);
    // REP MOVSW count8 replaces XYZ,yaw,pitch. Observer roll is retained.
    for(unsigned i=0;i<16;++i)wb(0xb05+i,b(0x285+i));
    mission_matrices();
    if(dotiles())return 2;
    const unsigned flags[]={0xe56,0xf7b,0xf79,0xf77,0xf57,0xf52,0xf55};for(unsigned p:flags)wb(p,0);
    wb(0x19bc,255);sw(0x1de2,u(0x1de6));
    unsigned divisor=u(0x1de4);uint32_t dividend=uint32_t(u(0x1de2))*32768u;
    if(!divisor||dividend/divisor>65535)return 2;
    sw(0x1e5c,uint32_t((dividend/divisor)*u(0x1e24))>>16);return 1;
}
extern "C" uint32_t cc_doxp(){
    events[0]=0;if(!b(0x8f88)){wb(0x8f88,0);return 0;}
    if(flush_tiles())return 2;
    empty_drawnear();event(5);
    if(!(b(0x8f88)&128)){for(unsigned i=0;i<12;++i)wb(0xb05+i,b(0x2a9+i));}
    else{sw(0xb07,int(random_bound(256))-128);sw(0xb0b,int(random_bound(256))-128);sw(0xb05,random_word());sw(0xb09,random_word());}
    sw(0xb0d,0x8000);sw(0xb0f,3);if(dotiles())return 2;
    wb(0x8f89,b(0x8f89)&253);wb(0x8f88,0);return 1;
}
