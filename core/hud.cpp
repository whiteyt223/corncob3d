#include "hud.hpp"
#include "state.hpp"
#include "runtime.hpp"
#include "bios_font14.hpp"
using namespace cc;using namespace cc::state;
extern "C" uint8_t* cc_video_memory();
namespace {
uint32_t radio[6]={};
bool cockpit(){return u(u(0x1c3e)+6)<210;}
void bytepaint(unsigned a,unsigned mask,unsigned color,int search=-1){
    auto* p=cc_video_memory()+uint16_t(a)*8;
    for(unsigned i=0;i<8;++i)if((mask&(128u>>i))&&(search<0||p[i]==unsigned(search)))p[i]=uint8_t(color&15);
}
void button_span(unsigned x,unsigned x2,unsigned y,unsigned offset,unsigned search,unsigned color){
    unsigned rem=uint16_t(x2-x-8+(x&7)),a=uint16_t(y*80+(x>>3)+offset);
    sw(0x2154,rem);
    bytepaint(a++,b(0x2c1d)>>(x&7),color,int(search));
    for(unsigned i=0;i<rem/8;++i)bytepaint(a++,b(0x2c1d),color,int(search));
    bytepaint(a,(0xff00u>>(rem&7))&255,color,int(search));
}
void damage_solid(unsigned p){
    unsigned index=u(p+4),x=uint16_t(index*6+u(p)),y=u(p+2);
    sw(0x1b83,x);sw(0x1b87,x+5);sw(0x1b85,y);sw(0x1b89,y);
    unsigned color=u(p+8)?b(0x1b8b):b(0xf30b+index);
    // rawhline short-span branch, five pixels, right endpoint excluded.
    unsigned mask=(0xf800u>>(x&7))&u(0x2c1e);
    const unsigned offsets[]={unsigned(u(0x1b90)),unsigned(u(0x1b90)^0x7e00)};
    for(unsigned offset:offsets){
        unsigned a=uint16_t(y*80+(x>>3)+offset);bytepaint(a,mask>>8,color);bytepaint(a+1,mask&255,color);
    }
    index=uint16_t(index+1);sw(p+4,index);
    if(w(index)>12){sw(p+4,12);if(!u(p+8)){sw(p+6,raw_ticks());sw(p+8,512);}}
}
bool record_valid(unsigned p){
    if(p>65535-16)return false;
    for(unsigned i=0;i<16;i+=8){unsigned x=u(p+i),y=u(p+i+2),width=u(p+i+4)&~1u,height=u(p+i+6);if(!height||x+width>80||y+height>350)return false;}
    unsigned cursor=p+16;
    for(unsigned line=0;line<64;++line){if(cursor>65533)return false;if(b(cursor)&128)return true;unsigned x=b(cursor),y=b(cursor+1);cursor+=2;if(x>=80||y>=25)return false;
        for(unsigned col=0;col<80;++col){if(cursor>=65535)return false;unsigned ch=b(cursor++);if(!ch)break;if(x+col>=80)return false;}
    }
    return false;
}
void draw_record(unsigned p){
    sw(0xf81,p);
    for(unsigned i=0;i<16;i+=8){unsigned x=u(p+i),y=u(p+i+2),width=u(p+i+4)&~1u,height=u(p+i+6),color=i?0:4;wb(0x1b8b,color);
        for(unsigned row=0;row<height;++row)for(unsigned col=0;col<width;++col)bytepaint((y+row)*80+x+col,255,color);
    }
    unsigned cursor=p+16;
    while(!(b(cursor)&128)){sw(0xf81,cursor);unsigned x=b(cursor),y=b(cursor+1);cursor+=2;
        for(unsigned col=0;col<80;++col){unsigned ch=b(cursor++);if(!ch)break;
            for(unsigned row=0;row<14;++row){unsigned a=(y*14+row)*80+x+col,bits=cc_bios_font14[ch*14+row];bytepaint(a,255,0);bytepaint(a,bits,7);}
        }
    }
}
void pair(unsigned kind,unsigned a,unsigned z){cc_hud_button(kind,a);cc_hud_button(kind,z);}
void radio_close(){
    unsigned code=b(0x26c)&127;if(code==0x18)wb(0xaf4,0);wb(0x26c,0);sw(0x1b90,radio[2]);
    radio[4]=(radio[2]&0x7000)?1:0; // original leaves physical page0 visible if old draw offset7E00
    if(radio[5])sw(0xf7f,0);
    radio[0]=0;
}
}
extern "C" uint32_t cc_hud_button(uint32_t index,uint32_t packed){
    if(index>=14)return 2;
    if(!cockpit())return 0;
    unsigned p=0xf29b+index*8,x=u(p),y=u(p+2),x2=u(p+4),y2=u(p+6);
    if(x>=640||x2<=x||x2>640||y2<y||y2>=350||x2-x+(x&7)<8)return 2;
    sw(0x1b83,x);sw(0x1b87,x2);
    const unsigned offsets[]={unsigned(u(0x1b90)),unsigned(u(0x1b90)^0x7e00)};
    for(unsigned offset:offsets)for(unsigned row=y;row<=y2;++row)button_span(x,x2,row,offset,packed&15,(packed>>8)&15);
    return 0;
}
extern "C" uint32_t cc_hud_damage(uint32_t kind){
    if(kind>3)return 2;
    if(!cockpit())return 0;
    unsigned p=0xf317+10*kind;
    if(u(p+8)){if(u(p+8)>=256)sw(p+8,u(p+8)>>1);return 0;}
    if(u(p+4)>12)return 2;
    damage_solid(p);return 0;
}
extern "C" uint32_t cc_hud_flash_damage(uint32_t kind){
    if(kind>3)return 2;
    if(!cockpit())return 0;
    unsigned p=0xf317+10*kind;if(!u(p+8))return 0;
    if(u(p+4)>12)return 2;
    wb(0x1b8b,((uint16_t(raw_ticks()-u(p+6)))&u(p+8))?14:12);damage_solid(p);return 0;
}
extern "C" uint32_t cc_hud_damage_indicator_init(uint32_t kind){
    if(kind>3)return 0xffffffffu;
    unsigned p=0xf317+10*kind,v=u(p+4);sw(p+4,0);sw(p+6,0);sw(p+8,0);return v;
}
extern "C" void cc_hud_damage_init(){
    if(b(0xae6))cc_hud_button(13,0x0700);
    else for(int kind=3;kind>=0;--kind){unsigned count=cc_hud_damage_indicator_init(kind);if(!b(0xf50))count=1;for(unsigned i=0;i<count;++i)cc_hud_damage(kind);}
    for(unsigned i=6;i<=8;++i)cc_hud_button(i,0x0807);
}
extern "C" void cc_hud_redraw_buttons(){
    if(b(0xf59))pair(2,0x0f07,0x0600);
    if(b(0xe59))pair(0,0x0f07,0x0100);
    if(b(0xf3c))pair(5,0x0f07,0x0900);
    if(b(0xe4e)<=3)for(unsigned i=b(0xe4e);i<3;++i)cc_hud_button(6+i,0x0708);
}
extern "C" void cc_hud_flash_alt(){cc_hud_button(9,0x0f00);if(!(raw_ticks()&128))pair(9,0x0f00,0x070e);else pair(9,0x000f,0x0e07);}
extern "C" void cc_hud_flash_eject(){cc_hud_button(4,0x0400);if(!(raw_ticks()&512))pair(4,0x0400,0x070f);else pair(4,0x0004,0x0f07);}
extern "C" void cc_hud_flash_stall(uint32_t r){if(uint16_t(r)>16000)pair(3,0x0e07,0x0c00);else pair(3,0x070e,0x000c);}
extern "C" void cc_hud_altitude_warning(){
    unsigned threshold=b(0xe45)?5000:4500;
    if(!u(0xb0f)&&u(0xb0d)<threshold&&!b(0xf77)&&!b(0xafc)){
        cc_hud_flash_alt();if(b(0xaf4)&&!b(0xf52)&&!b(0xe45)&&u(0xb0d)>=2000)sw(0xf7f,0x14ae);wb(0xe45,255);
    }else{wb(0xe45,0);pair(9,0x0f00,0x070e);}
}
extern "C" void cc_hud_complete_button(){if(b(0xf3c)&128){pair(5,0x0f07,0x0900);wb(0xf3c,1);}}
extern "C" uint32_t cc_radio_draw(uint32_t p){if(p>65535||!record_valid(p))return 2;draw_record(p);return 0;}
extern "C" uint32_t* cc_radio_state(){return radio;}
extern "C" void cc_radio_reset(){for(auto& v:radio)v=0;}
extern "C" uint32_t cc_radio_open(uint32_t p,uint32_t clearqueue){
    if(radio[0])return 1;
    if(p>65535||!record_valid(p))return 2;
    radio[0]=1;radio[1]=p;radio[2]=u(0x1b90);radio[3]=raw_ticks();radio[4]=1;radio[5]=bool(clearqueue);
    sw(0x1b90,0);draw_record(p);wb(0x1c9,255);wb(0x26d,0);return 1;
}
extern "C" uint32_t cc_radio_begin_queued(){
    if(radio[0])return 1;
    if(b(0x26c)==0xc4)sw(0xf7f,0x102f);
    if(!u(0xf7f)||!b(0xaee))return 0;
    return cc_radio_open(u(0xf7f),1);
}
extern "C" uint32_t cc_radio_update(){
    if(!radio[0])return 0;
    if(radio[0]==1){if(uint16_t(raw_ticks()-radio[3])<100||!b(0x26d)||(b(0x26c)&128))return 1;
        if((b(0x26c)&127)==0x4e){radio_close();return 0;}wb(0x26c,0);wb(0x26d,0);radio[0]=2;return 1;}
    if(!b(0x26d))return 1;
    unsigned code=b(0x26c);if(code==0x4e){radio_close();return 0;}if(!(code&128))return 1;
    if((code&127)==0x3c)wb(0x1ca,1);else if((code&127)==0x3e)wb(0xe46,255);radio_close();return 0;
}
