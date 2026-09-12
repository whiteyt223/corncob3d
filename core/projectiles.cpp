#include "audio.hpp"
#include "state.hpp"
#include "runtime.hpp"
using namespace cc;using namespace cc::state;
namespace {
bool valid_object(unsigned p){return p<=65536-74;}
uint32_t table_add(unsigned table,unsigned object){
    sw(0x2955,table);
    unsigned count=u(table),code=b(object+18);if(code&128)code=0;wb(object+18,code&3);
    wb(0x2957,code);
    // Valid original tables fit in their data segment. Report malformed host
    // inputs instead of allowing their insertion loop to wrap indefinitely.
    if(table+4+(count+1)*2>65536)return 2;
    uint32_t distance=uint32_t(l(object+20));unsigned index=0;
    for(;index<count;++index){unsigned other=u(table+4+index*2);if(code>b(other+18)||(code==b(other+18)&&distance>=uint32_t(l(other+20)))){if(object==other&&code==b(other+18)&&distance==uint32_t(l(other+20)))source_error(28);break;}}
    for(unsigned i=count;i>index;--i)sw(table+4+i*2,u(table+4+(i-1)*2));
    sw(table+4+index*2,object);sw(table,count+1);if(w(count+1-u(table+2))>=0)source_error(10);return 0;
}
unsigned find_bullet(){
    unsigned p=u(0x1c9d);
    for(unsigned n=0;n<32;++n){if(!u(p+24)&&(b(p+18)&128)){sw(0x1c9d,p);return p;}p+=74;if(p>=0x7dd8)p=0x7732;}
    return 0;
}
unsigned find_effect(){for(unsigned p=0x8fa6;p<0xec26;p+=74)if(!u(p+24)&&(b(p+18)&128)){sw(0x8fa4,p);return p;}return 0;}
void copy(unsigned from,unsigned to,unsigned size){for(unsigned i=0;i<size;++i)wb(to+i,b(from+i));}
void launch(unsigned p,unsigned matrix,unsigned origin,int speed,int side){
    for(unsigned a=0;a<3;++a){sw(p+46+a*2,hm(speed,s(matrix+a*6)));sl(p+a*4,d(int64_t(l(origin+a*4))+hm(side,s(matrix+a*6+2))));}
}
}
extern "C" uint32_t cc_table_add(uint32_t table,uint32_t object){if(!valid_object(object)||table>65532)return 2;return table_add(table,object);}
extern "C" uint32_t cc_table_remove(uint32_t table,uint32_t object){
    if(!valid_object(object)||table>65532||table+4+u(table)*2>65536)return 2;
    sw(0x2955,table);unsigned count=u(table),index=0;while(index<count&&u(table+4+index*2)!=object)++index;
    if(index==count){source_error(11);return 1;}
    wb(object+18,255);for(unsigned i=index;i+1<count;++i)sw(table+4+i*2,u(table+4+(i+1)*2));sw(table,count-1);return 0;
}
extern "C" uint32_t cc_table_sort(uint32_t table){
    if(table>65532||table+4+u(table)*2>65536)return 2;
    sw(0x2955,table);unsigned count=u(table);
    if(count<2)return 0;
    for(unsigned pass=0;pass<count;++pass){bool changed=false;
        for(unsigned i=0;i+1<count;++i){unsigned a=u(table+4+i*2),bobj=u(table+6+i*2);wb(0x2957,b(a+18));
            if(b(a+18)<b(bobj+18)||(b(a+18)==b(bobj+18)&&uint32_t(l(a+20))<uint32_t(l(bobj+20)))){sw(table+4+i*2,bobj);sw(table+6+i*2,a);changed=true;}
        }if(!changed)break;
    }
    // Original sort leaves the final compared BX entry's low distance-code
    // byte in odcode. Both release binaries preserve the adjacent high byte.
    wb(0x2957,b(u(table+2+count*2)+18));return 0;
}
extern "C" uint32_t cc_new_effect(uint32_t type){
    if(type>=61)return 0;
    unsigned prototype=u(0x2cca+type*2);sw(0xec28,prototype);
    if(!valid_object(prototype))return 0;
    unsigned p=find_effect();if(!p)return 0;copy(prototype,p,74);sw(0xec28,p);return p;
}
extern "C" void cc_clear_bullets(){sw(0x1c9d,0x7732);for(unsigned i=0;i<32;++i){unsigned p=0x7732+i*74;sw(p+24,0);wb(p+18,255);}}
extern "C" uint32_t cc_bullet_spawn(){
    if(!s(0x1e5e)||!s(0x1b7f))return 0;
    unsigned matrix=0x1d48,origin=0xe8b;sw(0xf43,matrix);sw(0xf45,origin);
    if(!s(0x1cff))sw(0x1cff,720);
    if(b(0xafc)&&!b(0xf54)){
        matrix=0x1ff0;origin=0xb05;sw(0xf43,matrix);sw(0xf45,origin);sw(0x1cff,0);sw(0x1ca3,33);sw(0x1d01,s(0x1d01)-1);
        if(s(0x1d01)<0){sw(0x1d01,0);return 0;}
    }
    cc_audio_shot(1);unsigned p=find_bullet();if(!p)return 0;
    launch(p,matrix,origin,w(s(0x1dc8)+s(0x1e3c)),s(0x1cff));
    sw(p+52,uwa(s(0x1e3e)));wb(p+18,0);sw(0x1cff,-s(0x1cff));sw(p+24,b(0xafc)&&b(0xf54)?1:9);
    table_add(u(0x2393),p);sw(0x1ca3,s(0x1ca3)+2);return p;
}
extern "C" uint32_t cc_missile_spawn(){
    unsigned prototype=0x855e;sw(0xec28,prototype);unsigned p=find_effect();if(!p)return 0;
    copy(prototype,p,74);sw(0xec28,p);wb(p+18,0);sw(p+52,100);sw(p+24,u(p+24)|0x8000);
    launch(p,0x1d48,0xe8b,w(s(0x1dc8)+125),s(0x1d04));copy(0xe97,p+12,6);sw(0x1d04,-s(0x1d04));return p;
}
// Return original carry as bit0. Bit1 reports an invalid host arithmetic
// domain. Ground-puff creation is a subsequent effect-chain operation.
extern "C" uint32_t cc_projectile_move(uint32_t p,uint32_t bullet){
    if(!valid_object(p)||!s(0x1b7f))return 2;
    unsigned dispersion=u(0x1ce),drag=u(0x1d0),gravity=u(0x1d2);bool rotation=b(p+26)!=0;
    if(!bullet&&rotation)for(unsigned a=0;a<3;++a)sw(p+12+a*2,s(p+12+a*2)+s(p+54+a*2));
    for(unsigned a=0;a<3;++a){
        int noise=0;
        if(bullet)noise=int(random_bound(dispersion))-int(dispersion/2);
        else if(dispersion)noise=int(random_bound(dispersion))-int((dispersion+1)/2);
        int velocity=w(s(p+46+a*2)+noise);
        if(!bullet&&drag){unsigned quotient=unsigned(velocity*velocity)/drag;if(quotient>65535)return 2;velocity=w(velocity+(velocity<0?int(quotient):-int(quotient)));}
        sw(p+46+a*2,velocity);int delta=velocity;
        if(a==2){delta=w(velocity-(bullet?s(0x1d2):gravity==55?55:wa(gravity)));if(!bullet&&gravity!=55)sw(p+50,delta);}
        addl(p+a*4,dwa(delta));
    }
    if(l(p+8)<0&&(bullet||s(p+50))){
        sl(p+8,0);for(unsigned i=46;i<52;i+=2)sw(p+i,0);
        if(bullet)sw(p+52,0);else if(rotation)for(unsigned i=54;i<66;++i)wb(p+i,0);
        return 1;
    }
    return 0;
}
