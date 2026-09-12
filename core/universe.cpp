#include "state.hpp"
#include "runtime.hpp"
#include "universe_fields.hpp"
using namespace cc;using namespace cc::state;namespace f=universe_field;
extern "C" uint32_t cc_table_remove(uint32_t,uint32_t);
extern "C" uint32_t cc_enemy_distance_object(uint32_t);
namespace {
uint8_t universe[0xff90+64*3456];
unsigned read_word(unsigned p){return unsigned(universe[p])|(unsigned(universe[p+1])<<8);}
void write_word(unsigned p,unsigned value){universe[p]=uint8_t(value);universe[p+1]=uint8_t(value>>8);}
bool valid_tile(int x,int y){return x>=0&&x<8&&y>=0&&y<8;}
unsigned tile_address(int x,int y){return 0xff90+unsigned(y*8+x)*3456;}
void cache_to_state(unsigned p,unsigned target){for(unsigned i=0;i<74;++i)wb(target+i,universe[p+i]);}
void state_to_cache(unsigned source,unsigned p){for(unsigned i=0;i<74;++i)universe[p+i]=b(source+i);}
unsigned free_cache(){for(unsigned p=0;p<0xff88;p+=74)if(!read_word(p+24)){sw(f::cache_exptr,p);return p;}return 65535;}
unsigned free_active(){for(unsigned p=0x8fa6;p<0xec26;p+=74)if(!u(p+24)&&(b(p+18)&128)){sw(f::exps_exptr,p);return p;}return 0;}
bool in_tile(unsigned p,int x,int y){int dx=w(s(p+2)-w(x*32+s(f::universex))),dy=w(s(p+6)-w(y*32+s(f::universey)));return dx>=0&&dx<32&&dy>=0&&dy<32;}
void current_tile(int x,int y){sw(f::curtilex,x);sw(f::curtiley,y);sw(f::tileseg,u(f::memseg)+0xff9+y*1728+x*216);}
}
extern "C" uint8_t* cc_universe_memory(){return universe;}
extern "C" uint32_t cc_decode_object(uint32_t target,int32_t x,int32_t y,uint32_t index){
    if(target>65536-74||!valid_tile(x,y)||index>=150)return 2;
    unsigned p=tile_address(x,y)+index*23,flags=read_word(p+11),type=read_word(p+21);if(!flags)return 1;
    sw(target+24,flags);sw(target+72,type);if(type>u(f::ntemplates)){source_error(39);return 1;}
    unsigned prototype=u(f::templatePointers+type*2);for(unsigned i=0;i<74;++i)wb(target+i,b(prototype+i));
    unsigned bits=read_word(p);
    sl(target,d((uint32_t(uint16_t(x*32+s(f::universex)+int(bits&31)))<<16)|read_word(p+2)));
    sl(target+4,d((uint32_t(uint16_t(y*32+s(f::universey)+int((bits>>5)&31)))<<16)|read_word(p+4)));
    sl(target+8,d((uint32_t((bits>>10)&31)<<16)|read_word(p+6)));
    for(unsigned a=0;a<3;++a){sw(target+12+a*2,unsigned(universe[p+8+a])<<8);sw(target+46+a*2,read_word(p+13+a*2));}
    sw(target+24,flags);sw(target+52,read_word(p+19));return 0;
}
extern "C" uint32_t cc_encode_object(uint32_t source,int32_t x,int32_t y,uint32_t index){
    if(source>65536-74||!valid_tile(x,y)||index>=150)return 2;
    unsigned p=tile_address(x,y)+index*23;
    unsigned hx=uint16_t(s(source+2)-x*32-s(f::universex))&31,hy=uint16_t(s(source+6)-y*32-s(f::universey))&31;
    write_word(p,hx|(hy<<5)|((u(source+10)&31)<<10));
    for(unsigned a=0;a<3;++a){write_word(p+2+a*2,u(source+a*4));universe[p+8+a]=b(source+13+a*2);write_word(p+13+a*2,u(source+46+a*2));}
    write_word(p+11,u(source+24));write_word(p+19,u(source+52));write_word(p+21,u(source+72));
    if(u(source+72)>u(f::ntemplates)){source_error(40);return 1;}return 0;
}
extern "C" uint32_t cc_distance_object(uint32_t object){
    if(object>65536-74)return 2;
    if(u(f::z0))return cc_enemy_distance_object(object);
    unsigned mask=0;int delta[3];
    for(unsigned a=0;a<3;++a){delta[a]=d(int64_t(l(object+a*4))-l(f::ox_oy_oz+a*4));sl(f::dxvec+a*4,delta[a]);uint32_t mag=delta[a]<0?0u-uint32_t(delta[a]):uint32_t(delta[a]);mask|=(mag*2u)>>16;}
    if(mask&0xff00){wb(object+18,3);return 1;}
    unsigned code=mask&0xf0?2:mask?1:0,shift=code*4;uint32_t square=0;
    for(int value:delta){int reduced=w(sar(value,shift));square+=uint32_t(reduced*reduced);}
    wb(object+18,code);sl(object+20,d(square));return 0;
}
extern "C" uint32_t cc_bring_tile(int32_t x,int32_t y){
    if(!valid_tile(x,y))return 2;
    current_tile(x,y);sw(f::tileptr,0);sw(f::tobjcount,0);unsigned index=0;
    for(unsigned p=0;p<0xff88;p+=74)if(!read_word(p+24)){
        if(index>=150)return 2;
        unsigned status=cc_decode_object(f::memobjbuf,x,y,index);if(status)return status==1?0:status;
        state_to_cache(f::memobjbuf,p);++index;sw(f::tileptr,index*23);
    }return 1;
}
extern "C" uint32_t cc_farm_tile(int32_t x,int32_t y){
    if(!valid_tile(x,y))return 2;
    current_tile(x,y);sw(f::tileptr,0);sw(f::tobjcount,0);unsigned index=0;
    for(unsigned p=0;p<0xff88;p+=74)if(read_word(p+24)){
        cache_to_state(p,f::memobjbuf);if(!in_tile(f::memobjbuf,x,y))continue;
        sw(f::tobjcount,u(f::tobjcount)+1);if(u(f::tobjcount)>=149){sw(f::tobjcount,u(f::tobjcount)-1);continue;}
        write_word(tile_address(x,y)+(index+1)*23+11,0);
        if(cc_encode_object(f::memobjbuf,x,y,index)){source_error(41);continue;}
        ++index;sw(f::tileptr,index*23);write_word(p+24,0);
    }return 1;
}
extern "C" void cc_update_tiles(){
    int x=w(w(s(f::ox_oy_oz+2)-s(f::universex))+16)/32,y=w(w(s(f::ox_oy_oz+6)-s(f::universey))+16)/32;sw(f::ctx,x);sw(f::cty,y);
    for(unsigned i=0;i<4;++i){int a=s(f::ctiles+i*4),bval=s(f::ctiles+i*4+2);if(valid_tile(a,bval)&&((uint16_t(x-a)&0xfffe)||(uint16_t(y-bval)&0xfffe))){cc_farm_tile(a,bval);wb(f::ctstatus+bval*8+a,0);}}
    for(unsigned i=0;i<4;++i){int a=w(x+s(f::ctileoffs+i*4)),bval=w(y+s(f::ctileoffs+i*4+2));sw(f::ctiles+i*4,a);sw(f::ctiles+i*4+2,bval);}
    for(unsigned i=0;i<4;++i){int a=s(f::ctiles+i*4),bval=s(f::ctiles+i*4+2);if(valid_tile(a,bval)&&!b(f::ctstatus+bval*8+a)){cc_bring_tile(a,bval);wb(f::ctstatus+bval*8+a,255);}}
}
// Complete one original drawmemseg iteration after drawing memobjbuf. The
// renderer owns the distance evaluation and shared RNG before this handoff.
extern "C" uint32_t cc_promote_cached(uint32_t p){
    if(p>=0xff88||p%74)return 2;
    if(b(f::memobjbuf+18)>=2)return 0;
    sw(f::memobjbuf+24,u(f::memobjbuf+24)|0x8000);sw(f::fdexpobj,f::memobjbuf);unsigned target=free_active();
    if(target){for(unsigned i=0;i<74;++i)wb(target+i,b(f::memobjbuf+i));}else source_error(63);
    write_word(p+24,0);return target?0:1;
}
extern "C" uint32_t cc_promote_objects(){
    for(unsigned p=0;p<0xff88;p+=74)if(read_word(p+24)){
        cache_to_state(p,f::memobjbuf);cc_distance_object(f::memobjbuf);
        if(cc_promote_cached(p))return 1;
    }return 0;
}
extern "C" uint32_t cc_cache_far_objects(){
    unsigned table=u(f::tblist),count=u(table);
    if(table+4+count*2>65536)return 2;
    sw(f::tbstrt,table);
    for(unsigned i=0;i<count;++i){unsigned object=u(table+4);if(b(object+18)<2)continue;
        sw(f::fdexpobj,object);
        if(!(u(f::extendedtypetbl+u(object+72)*2)&1)){unsigned p=free_cache();if(p==65535){source_error(37);continue;}state_to_cache(object,p);}
        sw(object+24,0);cc_table_remove(table,object);
    }return 0;
}
