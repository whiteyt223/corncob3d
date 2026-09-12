#include "fixed.hpp"
using namespace cc;
extern "C" {int32_t cc_ssin(uint32_t);int32_t cc_scos(uint32_t);uint32_t cc_atn2(uint32_t,uint32_t);uint8_t* cc_framebuffer();}
namespace {
// oz,hmin,yaw,pitch,roll,speed,side,vertical,climb,rpm,rpmx,temp,oil,flags,frame.
uint32_t input[15];
unsigned oil_color=13;
struct Command{uint32_t kind;int32_t xy[4];uint32_t color,mask,diameter;int32_t window[4];};
Command commands[24];unsigned count;
struct Cache{Command always[8],slow[4][2];};
Cache pages[2];
uint8_t video[65536*8];
int video_page=-1;
unsigned slow_group(unsigned frame){const unsigned groups[14]={0,0,1,1,2,2,3,3,1,1,2,2,3,3};return groups[frame%14];}
unsigned slow_count(unsigned group){return group==1?2:1;}
void load_page(unsigned page){const unsigned start=page?0:0x7e00*8;auto* f=cc_framebuffer();for(unsigned i=0;i<640*350;++i)f[i]=video[start+i];}
int rp(int a,int b){return w(sar(a*b+32768,16));}
void line(int x,int y,int x2,int y2,unsigned color=8,unsigned mask=8){commands[count++]={0,{x,y,x2,y2},color,mask,0,{0,0,0,0}};}
void needle(int angle,int x,int y,int xs,int ys,unsigned color=8,unsigned mask=8){line(x,y,w(x+rp(cc_ssin(uint16_t(angle)),xs)),w(y+rp(w(-cc_scos(uint16_t(angle))),ys)),color,mask);}
void bar(int angle,int x,int y,int xs,int ys,int* end=nullptr){int dx=rp(cc_scos(uint16_t(angle)),xs),dy=rp(cc_ssin(uint16_t(angle)),ys);line(x-dx,y-dy,x+dx,y+dy);if(end){end[0]=x+dx;end[1]=y+dy;}}
void disc(int x,int y,unsigned diameter,unsigned color,unsigned mask,int l,int t,int r,int b){commands[count++]={1,{w(x),w(y),0,0},color,mask,diameter,{l,t,r,b}};}
void point(int x,int y,unsigned color,unsigned mask){
    uint8_t* pixel;
    if(video_page>=0){unsigned address=uint16_t(y*80+sar(x,3)+(video_page?0:0x7e00));pixel=video+address*8+(unsigned(x)&7);}
    else{if(x<0||x>=640||y<0||y>=350)return;pixel=cc_framebuffer()+y*640+x;}
    *pixel=uint8_t((*pixel&(~mask&15))|(color&mask));
}
void paint(const Command& c,unsigned color){
    int x=c.xy[0],y=c.xy[1],x2=c.xy[2],y2=c.xy[3];
    if(c.kind==2){point(x,y,color,c.mask);return;}
    if(c.kind==1){
        if(x<c.window[0]||x>=c.window[2]||y<c.window[1]||y>=c.window[3])return;
        const uint16_t five[4]={0x1c0,0x3e0,0x3e0,0x1c0},six[5]={0x3c0,0x7e0,0x7e0,0x7e0,0x3c0};
        for(unsigned row=0;row<(c.diameter==5?4:5);++row){uint32_t bits=uint32_t(c.diameter==5?five[row]:six[row])<<8;bits>>=x&7;int base=(x>>3)*8-8;for(int bit=0;bit<24;++bit)if(bits&(1u<<(23-bit)))point(base+bit,y+int(row)-2,color,c.mask);}
        return;
    }
    if(x2<x){int t=x;x=x2;x2=t;t=y;y=y2;y2=t;}
    int dx=x2-x,dy=y2>=y?y2-y:y-y2,sy=y2>=y?1:-1,major=dx>=dy?dx:dy,minor=dx>=dy?dy:dx,error=2*minor-major;
    for(int i=0;i<=major;++i){point(x,y,color,c.mask);bool step=error>=0;if(dy<=dx){++x;if(step)y+=sy;}else{y+=sy;if(step)++x;}error+=2*minor-(step?2*major:0);}
}
int limited_angle(int v){v=w(v);if(v<0)v=0;if(w(v-10800)>=0)v=10799;return v;}
void clear_mds(){for(int y=255;y<=334;++y)for(int x=496;x<=615;++x)point(x,y,0,15);}
Command initial_line(int x,int y,unsigned color=8,unsigned mask=8){return {0,{x,y,0,0},color,mask,0,{0,0,0,0}};}
}
extern "C" uint8_t* cc_video_memory(){return video;}
extern "C" void cc_video_load(uint32_t page){load_page(page&1);}
extern "C" void cc_video_store(uint32_t page){const unsigned start=page&1?0:0x7e00*8;auto* f=cc_framebuffer();for(unsigned i=0;i<640*350;++i)video[start+i]=f[i];}
extern "C" void cc_instrument_reset(){
    video_page=-1;count=0;oil_color=13;
    for(auto& p:pages){
        p.always[0]=initial_line(43,267);p.always[1]=initial_line(43,318);p.always[2]=initial_line(116,318);p.always[3]=initial_line(232,636);
        p.always[4]={1,{0,246,0,0},8,8,5,{481,243,629,249}};
        p.always[5]=initial_line(79,224,13,15);p.always[5].xy[2]=79;p.always[5].xy[3]=224;
        p.always[6]=initial_line(43,267,9,15);p.always[7]=initial_line(43,267,7,15);
        p.slow[0][0]=initial_line(32,232);p.slow[1][0]=initial_line(348,448);p.slow[1][1]=initial_line(0,0);
        p.slow[2][0]=initial_line(127,224);p.slow[3][0]=initial_line(116,267);
    }
}
extern "C" void cc_instrument_oil_color(uint32_t color){oil_color=color&15;}
extern "C" uint32_t* cc_instrument_input(){return input;}
extern "C" uint32_t cc_instrument_draw(){
    if(!uint16_t(input[10]))return 2;
    count=0;
    uint32_t altitude=input[0]-uint16_t(input[1]);if(altitude&0x80000000u)altitude=0;
    uint64_t altq=(uint64_t(altitude)<<16)/3050;
    needle(uint16_t(altq),43,267,46,32);
    int climb=w(uint16_t(input[8])+804);if(climb<0)climb=0;if(w(climb-1608)>=0)climb=1607;
    needle(int((uint32_t(climb)<<16)/2414+27306),43,318,46,32);
    needle(uint16_t(input[4]),116,318,10,7);bar(uint16_t(input[4]),116,318,46,32);
    const unsigned phase=uint16_t(input[14])%14;
    if(phase<2)needle(int(uint16_t(input[11])/3)-5400,32,232,40,28);
    else if(phase==6||phase==7||phase==12||phase==13){uint32_t speed=uint16_t(input[5])*(input[13]&0x2000?4u:1u);needle(int(((speed%19648)<<16)/19648),116,267,46,32);}
    else if(phase==4||phase==5||phase==10||phase==11)needle(-int(uint16_t(input[2])),127,224,20,14);
    else {int a=w(-int(uint16_t(input[3])));if(w(uint16_t(input[4])+16384)<0)a=w(-a+32768);int end[2];bar(a,174,224,30,21,end);needle(a,end[0],end[1],10,7);}
    disc(int(uint32_t(uint16_t(input[9]))*148/uint16_t(input[10])+478),246,5,8,8,481,243,629,249);
    needle(int((uint16_t(input[12])/3)*4)-27367,79,224,20,14,oil_color,15);
    altq/=10;needle(uint16_t(altq),43,267,28,20,9,15);altq/=10;needle(uint16_t(altq),43,267,24,16,7,15);
    int y=limited_angle(-int(cc_atn2(input[5],input[7]))+5400)*110/10800-55+298;
    int x=limited_angle(int(cc_atn2(input[5],input[6]))+5400)*160/10800-80+559;
    disc(x,y,6,12,15,496,259,615,327);
    const int pts[8]={559,295,563,295,561,294,561,297};for(unsigned i=0;i<4;++i)commands[count++]={2,{pts[i*2],pts[i*2+1],0,0},7,15,0,{0,0,0,0}};
    for(unsigned i=0;i<count;++i)paint(commands[i],commands[i].color);
    return 0;
}
extern "C" void cc_instrument_erase(){
    for(unsigned i=0;i<count;++i)if(commands[i].kind!=2&&!(commands[i].kind==1&&commands[i].diameter==6))paint(commands[i],0);
    clear_mds();
}
extern "C" uint32_t cc_instrument_draw_page(uint32_t page){
    page&=1;video_page=int(page);uint32_t status=cc_instrument_draw();
    if(!status){auto& p=pages[page];unsigned group=slow_group(uint16_t(input[14])),n=slow_count(group);
        for(unsigned i=0;i<4;++i)p.always[i]=commands[i];
        for(unsigned i=0;i<n;++i)p.slow[group][i]=commands[4+i];
        for(unsigned i=0;i<4;++i)p.always[4+i]=commands[4+n+i];
    }
    video_page=-1;load_page(page);return status;
}
extern "C" void cc_instrument_erase_page(uint32_t page,uint32_t frame){
    page&=1;video_page=int(page);const auto& p=pages[page];unsigned group=slow_group(uint16_t(frame));
    for(unsigned i=0;i<4;++i)paint(p.always[i],0);
    for(unsigned i=0;i<slow_count(group);++i)paint(p.slow[group][i],0);
    const unsigned tail[4]={4,6,7,5};for(unsigned i:tail)paint(p.always[i],0);
    clear_mds();video_page=-1;load_page(page);
}
