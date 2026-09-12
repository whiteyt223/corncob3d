#include "stars.hpp"
#include "stars_initial.hpp"
#include "fixed.hpp"
#include "viewport.hpp"
using namespace cc;
extern "C" {uint8_t* cc_framebuffer();uint32_t cc_draw_disc(uint32_t,uint32_t,uint32_t,uint32_t);}
namespace {
uint8_t data[65536],video[65536*8];unsigned status=0,display=1;uint32_t palette[4];
unsigned b(unsigned p){return data[uint16_t(p)];}unsigned u(unsigned p){return b(p)|(b(p+1)<<8);}int s(unsigned p){return w(u(p));}
void wb(unsigned p,unsigned v){data[uint16_t(p)]=uint8_t(v);}void sw(unsigned p,int v){wb(p,v);wb(p+1,unsigned(v)>>8);}
uint32_t dw(unsigned p){return u(p)|(u(p+2)<<16);}void sd(unsigned p,uint32_t v){sw(p,v);sw(p+2,v>>16);}
unsigned rnd(unsigned range){uint32_t seed=uint32_t(uint64_t(dw(0x28c8))*dw(0x28cc));sd(0x28c8,seed);return (seed>>16)*range/65535;}
void milky(){for(unsigned i=0,p=0x2c8;i<u(0x61);++i,p+=10){sw(p,rnd(8192)-4096);sw(p+2,rnd(8192)-4096);sw(p+4,rnd(32767));unsigned color=7;if(b(0xc0)){color=(rnd(32)+1)&255;if(color>=8)color=7;}sw(p+8,color<<8);sw(p+6,-1);}}
void fill(unsigned address,unsigned bytes,unsigned rows,unsigned color){
    for(unsigned y=0;y<rows;++y)for(unsigned i=0;i<bytes;++i){unsigned a=uint16_t(address+y*80+i+u(0x67))*8;for(unsigned bit=0;bit<8;++bit)video[a+bit]=color&15;}
    sw(0x6a,0);
}
void clear(){
    if(!b(0x60)){unsigned win=u(0xb5);wb(0xd8,b(0xb9));fill(u(win+20),((u(win+8)+1)>>4)*2,u(win+10),b(0xb9));}
    wb(0xd8,b(0xba));fill(0x4f75,38,90,b(0xba));
}
void border(){
    unsigned win=u(0xb5),left=uint16_t(u(win+20)-1),height=u(win+10),color=b(0x6c);
    fill(left,1,height,color);fill(left+79,1,height,color);fill(0,640,1,color);fill(u(win+22),640,1,color);
    if(u(0xb7)){fill(0x4de4,40,5,color);fill(0x6b94,40,5,color);fill(0x4f74,1,91,color);fill(0x4f9b,1,91,color);}
}
void flip(){wb(0x69,b(0x69)^255);sw(0x67,b(0x69)?0:0x7e00);display=b(0x69)?0:1;}
void fade(){palette[0]=b(0xbb);for(unsigned i=0;i<3;++i)palette[i+1]=(b(0xbc+i)*b(0xbf))>>8;}
void disc(unsigned win,unsigned size){
    sw(0xc6,u(win+16));sw(0xc8,u(win+18));sw(0xca,size);wb(0xd8,2);
    uint16_t previous[12];for(unsigned i=0;i<12;++i){previous[i]=cc::view::active[i];cc::view::active[i]=u(win+i*2);}
    unsigned base=u(0x67)*8;auto* frame=cc_framebuffer();for(unsigned i=0;i<640*350;++i)frame[i]=video[base+i];
    cc_draw_disc(u(0xc6),u(0xc8),u(0xca),2);
    for(unsigned i=0;i<640*350;++i)video[base+i]=frame[i];
    cc::view::set(previous);
}
unsigned stars(){
    for(unsigned i=0,p=0x2c8;i<u(0x61);++i,p+=10){
        int z=w(s(p+4)+s(0x63));if(z<0)z=int(uint16_t(z)&0x7fff);sw(p+4,z);
        bool visible=true;int px=0,py=0,product=s(p)*3200;
        if(unsigned(z)<=3200&&unsigned(absw(product/640))>=unsigned(z))visible=false;
        if(visible){if(!z)return 3;px=w(product/z+640);if(px<0)visible=false;else{px=uint16_t(px)>>1;if(w(px-639)>=0)visible=false;}}
        if(visible){product=s(p+2)*3200;if(unsigned(z)<=3200&&unsigned(absw(product/457))>=unsigned(z))visible=false;}
        if(visible){py=w(product/z+200);if(py<0||unsigned(py)>=400)visible=false;else py=uint16_t(py)>>1;}
        if(!visible){if(int8_t(b(0x60))<0)sw(p+6,z);wb(p+8,255);continue;}
        unsigned address=uint16_t(py*80+(px>>3)+u(0x67)),mask=128u>>(px&7);wb(p+8,mask);sw(p+6,address);
        unsigned color=z>26000?8:(z<20000?8:0)+b(p+9);video[address*8+(px&7)]=color&15;
    }
    return 0;
}
void restart(unsigned tick){milky();sw(0xe1,tick);sw(0x5e,0);}
}
extern "C" uint8_t* cc_stars_state(){return data;}
extern "C" uint8_t* cc_stars_video(){return video;}
extern "C" uint32_t cc_stars_status(){return status;}
extern "C" uint32_t cc_stars_display_page(){return display;}
extern "C" uint32_t* cc_stars_palette(){return palette;}
extern "C" void cc_stars_present(){unsigned base=(display?0:0x7e00)*8;for(unsigned i=0;i<640*350;++i)cc_framebuffer()[i]=video[base+i];}
extern "C" void cc_stars_begin(uint32_t index,uint32_t r,uint32_t g,uint32_t blue,uint32_t tick){
    for(unsigned i=0;i<65536;++i)data[i]=i<sizeof(stars_initial)?stars_initial[i]:0;
    status=0;display=1;wb(0xc1,0);milky();wb(0xbb,index);wb(0xbc,r);wb(0xbd,g);wb(0xbe,blue);sw(0xe1,tick);sw(0x5e,0);
    palette[0]=index;palette[1]=r;palette[2]=g;palette[3]=blue;
}
extern "C" uint32_t cc_stars_frame(uint32_t key,uint32_t tick){
    if(status)return status;
    if(u(0x5e)<144)sw(0x5e,u(0x5e)+1);
    if(u(0x5e)<144){unsigned remaining=144-u(0x5e);sw(0x5c,remaining*remaining*1024u/20736);wb(0xbf,255-u(0x5e)*255u/144);}
    else if(b(0xc1)){if(!u(0x67))flip();return status=2;}
    else wb(0xbf,0);
    fade();clear();if(u(0x5e)>=72){unsigned error=stars();if(error)return status=error;}
    if(u(0x5e)<144)disc(u(b(0xc1)?0xb5:0xb7),b(0xc1)?100:u(0x5c));
    border();flip();sw(0xe1,tick);
    if(uint8_t(key)==27)return status=1;
    if(uint8_t(key)==13)wb(0x60,b(0x60)^255);
    if(b(0x60)){
        sw(0x65,u(0x65)+1);
        if(u(0x65)>270){wb(0xb9,11);wb(0xba,11);wb(0x60,0);for(unsigned i=0;i<3;++i)wb(0xbc+i,63);clear();flip();clear();wb(0xc1,255);restart(tick);}
    }
    return 0;
}
