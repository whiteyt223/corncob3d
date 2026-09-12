#include "world_io.hpp"
#include "edition.hpp"
#include "state.hpp"
#include "universe_fields.hpp"
using namespace cc;using namespace cc::state;namespace f=universe_field;
extern "C" {uint8_t* cc_universe_memory();uint32_t cc_farm_tile(int32_t,int32_t);uint32_t cc_bring_tile(int32_t,int32_t);}
namespace {
constexpr unsigned capacity=262144,worldSize=0x45f90;
uint8_t file[capacity];uint32_t result[4];
unsigned world_word(unsigned p){uint8_t* a=cc_universe_memory();return a[p]|(unsigned(a[p+1])<<8);}
void world_word(unsigned p,unsigned v){uint8_t* a=cc_universe_memory();a[p]=uint8_t(v);a[p+1]=uint8_t(v>>8);}
void crypt(unsigned p,unsigned count){for(unsigned i=0;i<count;++i,p+=23)world_word(p+21,world_word(p+21)^world_word(p+2)^world_word(p+4));}
void reset_result(unsigned offset){result[0]=0;result[1]=offset;result[2]=0;result[3]=0;}
uint32_t fail(unsigned status){result[0]=status;return status;}
// The original read_h does not treat EOF or a short successful read as CF=1.
// Preserve bytes beyond the returned DOS read count.
void read_ds(unsigned p,unsigned count,unsigned size){unsigned at=result[1],n=size-at<count?size-at:count;for(unsigned i=0;i<n;++i)wb(p+i,file[at+i]);result[1]+=n;}
void read_world(unsigned p,unsigned count,unsigned size){unsigned at=result[1],n=size-at<count?size-at:count;for(unsigned i=0;i<n;++i)cc_universe_memory()[p+i]=file[at+i];result[1]+=n;}
bool write_ds(unsigned p,unsigned count){if(result[1]>capacity-count)return false;for(unsigned i=0;i<count;++i)file[result[1]++]=b(p+i);return true;}
bool write_world(unsigned p,unsigned count){if(result[1]>capacity-count)return false;for(unsigned i=0;i<count;++i)file[result[1]++]=cc_universe_memory()[p+i];return true;}
bool select(unsigned i){int x=s(f::ctiles+i*4);sw(f::curtilex,x);if(x<0||x>=8)return false;int y=s(f::ctiles+i*4+2);sw(f::curtiley,y);return y>=0&&y<8;}
uint32_t transfer(bool bring){int x=s(f::curtilex),y=s(f::curtiley);unsigned p=u(f::cache_exptr);uint32_t status=bring?cc_bring_tile(x,y):cc_farm_tile(x,y);sw(f::cache_exptr,p);return status;}
}
extern "C" uint8_t* cc_world_file_buffer(){return file;}
extern "C" uint32_t cc_world_file_capacity(){return capacity;}
extern "C" uint32_t* cc_world_io_result(){return result;}
extern "C" uint32_t cc_world_read_file_mode(uint32_t size,uint32_t offset,uint32_t plain){
    reset_result(offset);if(size>capacity||offset>size)return fail(2);
    for(unsigned y=0;y<8;++y){sw(f::curtiley,y);for(unsigned x=0;x<8;++x){
        sw(f::curtilex,x);read_ds(f::header,16,size);
        // getheader's first CMP is overwritten: only bytes2..3 are checked.
        if(u(f::header+4)!=x||u(f::header+6)!=y||u(f::header+2)!=0x65)return fail(1);
        unsigned count=u(f::header+8),p=0xff90+(y*8+x)*3456;
        if(count>150||p+count*23>worldSize)return fail(2);
        read_world(p,count*23,size);if(!plain)crypt(p,count);++result[2];result[3]+=count;
    }}
    read_ds(f::startcoords,12,size);return 0;
}
extern "C" uint32_t cc_world_write_file_mode(uint32_t offset,uint32_t plain){
    reset_result(offset);if(offset>capacity)return fail(2);
    for(unsigned y=0;y<8;++y){sw(f::curtiley,y);for(unsigned x=0;x<8;++x){
        sw(f::curtilex,x);sw(f::header+4,x);sw(f::header+6,y);sw(f::header+8,0);
        unsigned p=0xff90+(y*8+x)*3456,count=0;
        while(count<149&&world_word(p+count*23+11)){++count;sw(f::header+8,count);}
        if(!write_ds(f::header,16))return fail(2);
        if(!plain)crypt(p,count);
        if(!write_world(p,count*23))return fail(2);
        ++result[2];result[3]+=count;
    }}
    return write_ds(f::startcoords,12)?0:fail(2);
}
extern "C" void cc_world_allocate(uint32_t segment){sw(f::memsegtotal,0x49f9);sw(f::memseg,segment);for(unsigned p=0;p<0x45f80;++p)cc_universe_memory()[p]=0;}
extern "C" void cc_world_clean_objects(){
    sw(f::nplaced,b(0xae8)||!b(0xaf1)?1:0);
    for(unsigned p=0x8fa6;p<0xec26;p+=74){sw(p+24,0);wb(p+18,255);}
}
extern "C" uint32_t cc_world_update_tiles(){
    sw(f::ctx,w(s(0xb07)-s(f::universex)+16)/32);sw(f::cty,w(s(0xb0b)-s(f::universey)+16)/32);
    for(unsigned i=0;i<4;++i)if(select(i)){
        int x=s(f::curtilex),y=s(f::curtiley);
        if((uint16_t(s(f::ctx)-x)&0xfffe)||(uint16_t(s(f::cty)-y)&0xfffe)){
            if(transfer(false)==2)return 2;
            wb(f::ctstatus+y*8+x,0);
        }
    }
    for(unsigned i=0;i<4;++i){sw(f::ctiles+i*4,s(f::ctx)+s(f::ctileoffs+i*4));sw(f::ctiles+i*4+2,s(f::cty)+s(f::ctileoffs+i*4+2));}
    for(unsigned i=0;i<4;++i)if(select(i)){
        unsigned p=f::ctstatus+s(f::curtiley)*8+s(f::curtilex);if(!b(p)){if(transfer(true)==2)return 2;wb(p,255);}
    }
    return 0;
}

extern "C" uint32_t cc_world_read_file(uint32_t size,uint32_t offset){return cc_world_read_file_mode(size,offset,cc_edition_is_other_worlds());}
extern "C" uint32_t cc_world_write_file(uint32_t offset){return cc_world_write_file_mode(offset,cc_edition_is_other_worlds());}
