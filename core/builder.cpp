#include "builder.hpp"
#include "state.hpp"
#include "runtime.hpp"
#include "universe_fields.hpp"
#include "world_transition.hpp"
using namespace cc;using namespace cc::state;namespace uf=universe_field;
extern "C" {
void cc_startup_recalcmats(uint8_t*);
void cc_matvmul(const uint16_t*,const uint16_t*,uint16_t*);
uint32_t cc_map_bump_angles(int32_t,int32_t);
uint32_t cc_table_add(uint32_t,uint32_t);uint32_t cc_table_remove(uint32_t,uint32_t);uint32_t cc_table_sort(uint32_t);
uint32_t cc_distance_object(uint32_t);uint32_t cc_cache_far_objects();void cc_update_tiles();
}
namespace {
constexpr unsigned cursor=0xff10,buf=0xf4c3,ptr=0xf50d,save=0xf50f,type=0xf521,qual=0xf523,closest=0xf524,speed=0xf526,nomove=0xf528;
constexpr unsigned ox=0xb05,angles=0xb11,vector=0x1d06,surface=0x1da8,matrix=0x1ff0;
bool active=false;unsigned phase=0,scenes=0,near_table=0,near_slot=0,near_remaining=0,near_object=0;bool near_had=false;
int32_t private_state[8]={-1,15,0,0,0,0,0,0}; // defaultz,color,letter,x,y,axesX,axesY,beep events
bool key(unsigned k){return (b(0x2c7+k/8)&(1u<<((-k)&7)))!=0;}
void copy(unsigned from,unsigned to,unsigned count){for(unsigned i=0;i<count;++i)wb(to+i,b(from+i));}
void mats(){cc_startup_recalcmats(cc_flight_state());}
void beep(){++private_state[7];}
bool valid(unsigned p){return p<=65536-74;}
int clipped(int x){return uint16_t(absw(w(x)))<=32?0:w(x);}
void transform(){uint16_t m[9],v[3],out[3];for(unsigned i=0;i<9;++i)m[i]=u(matrix+i*2);for(unsigned i=0;i<3;++i)v[i]=u(vector+i*2);cc_matvmul(m,v,out);for(unsigned i=0;i<3;++i)sw(surface+i*2,out[i]);}
void move_object(){transform();unsigned p=u(ptr);for(unsigned i=0;i<3;++i)addl(p+i*4,s(surface+i*2));}
void show_cursor(){unsigned p=u(ptr);if(u(p+24))return;copy(ox,p,18);sw(vector,32767);sw(vector+2,0);sw(vector+4,0);move_object();sw(p+24,1);cc_table_add(u(uf::tblist),p);}
unsigned promote_buffer(){
    sw(buf+24,u(buf+24)|0x8000);sw(uf::fdexpobj,buf);
    for(unsigned p=0x8fa6;p<0xec26;p+=74)if(!u(p+24)&&(b(p+18)&128)){sw(uf::exps_exptr,p);copy(buf,p,74);return p;}
    source_error(63);return 0xec26; // exhausted320-slot findexp leaves SI at the pool end
}
unsigned nearest(){
    unsigned table=u(uf::tblist),count=u(table);if(!count)return 0;
    unsigned n=uint16_t(count-u(closest));if(count<=u(closest))sw(closest,0);
    unsigned at=uint16_t(table+2+uint16_t(n*2));
    if(u(at)==u(ptr)){if(uint16_t(n*2)<=2)return 0;at=uint16_t(at-2);}
    return u(at);
}
void prepare_nearest(){copy(ox,save,18);copy(u(ptr),ox,18);wb(nomove,255);scenes=3;phase=1;}
uint32_t finish_request(){wb(0x26c,0);for(unsigned i=0;i<3;++i)sw(surface+i*2,0);sw(cursor+24,0);phase=3;return 1;}
uint32_t bump_object(int pitch,int yaw){
    const unsigned p=u(ptr);for(unsigned i=0;i<2;++i){int v=sar(w(i?yaw:pitch),1);if(!s(0x1b7f))return 3;int n=v*s(0x1e5e)/s(0x1b7f);if(n<-32768||n>32767)return 3;unsigned at=p+(i?12:14);sw(at,s(at)+w(n*2));}return 0;
}
}
extern "C" int32_t* cc_builder_private(){return private_state;}
extern "C" void cc_builder_reset(){active=false;phase=0;private_state[0]=-1;private_state[1]=15;for(unsigned i=2;i<8;++i)private_state[i]=0;}
extern "C" uint32_t cc_builder_active(){return active;}
extern "C" uint32_t cc_builder_object(){return u(ptr);}
extern "C" uint32_t cc_builder_cursor(){return cursor;}
extern "C" uint32_t cc_builder_nomove(){return b(nomove);}
extern "C" void cc_builder_axes(int32_t x,int32_t y){private_state[5]=w(x);private_state[6]=w(y);}
extern "C" void cc_builder_speed_key(){unsigned n=b(0x26c)&127;if(n>=2&&n<=11){sw(speed,n*n*400);wb(0x26c,0);}}
extern "C" void cc_builder_set_quality(uint32_t q){unsigned p=u(ptr);sw(p+24,(u(p+24)&0x8fff)|((q&7)<<12));}
extern "C" uint32_t cc_builder_get_quality(){return (u(u(ptr)+24)>>12)&7;}
extern "C" void cc_builder_quality_key(){
    unsigned p=u(ptr),scan=b(0x26c);if(!u(p+24))return;
    unsigned q=scan&128?uint8_t((scan&127)-59):255;
    if(q<8){wb(0x26c,0);wb(qual,q);cc_builder_set_quality(q);}
    else if(q==8){sw(p+24,u(p+24)^0x800);wb(0x26c,0);}
    else if(q==9||scan==9){sw(p+24,u(p+24)^0x200);wb(0x26c,0);}
}
extern "C" uint32_t cc_builder_valid_type(uint32_t t){t=uint16_t(t);return t>u(uf::ntemplates)?1:(u(uf::extendedtypetbl+t*2)>>1)&1;}
extern "C" uint32_t cc_builder_copy_template(uint32_t p,uint32_t t){t=uint16_t(t);if(t>u(uf::ntemplates))return 1;copy(u(uf::templatePointers+t*2),p,74);return 0;}
extern "C" void cc_builder_begin(){active=true;phase=0;sw(ptr,cursor);sw(cursor+24,0);wb(0x26c,0x82);cc_builder_speed_key();}
extern "C" uint32_t cc_builder_begin_frame(){
    if(cc_runtime_state()[1])return 3;unsigned p=u(ptr);if(!valid(p))return 3;
    if(u(p+24)){if(b(0x26c)==0xa3){wb(0x26c,0);if(key(0x38))private_state[0]=l(p+8);else sl(p+8,private_state[0]);}sw(p+24,u(p+24)|0x400);}return 0;
}
extern "C" void cc_builder_after_frame(){sw(u(ptr)+24,u(u(ptr)+24)&0xfbff);}
extern "C" uint32_t cc_builder_zoom_object(uint32_t p){
    int direction=0;if(key(0x51)&&u(ox+10)<210)direction=1024;if(key(0x49))direction=-1024;
    if(direction){int32_t height=d((uint32_t(l(ox+8))>>4)+40),delta=dwa(mul(height,direction)/1024);if(key(0x1f))delta=delta<0?-1000:1000;addl(p+8,delta);}if(s(p+10)<0)sl(p+8,0);return 0;
}
extern "C" uint32_t cc_builder_move_cursor(int32_t x,int32_t y){
    const unsigned p=u(ptr);int old_pitch=s(angles+2);sw(angles+2,0);mats();int32_t old_z=l(p+8);
    int left=w(-clipped(x)),forward=w(-clipped(y)),twice=w(s(speed)*2);
    sw(vector+2,hi(w(left*32)*twice));sw(vector,hi(w(forward*32)*twice));sw(vector+4,0);move_object();sl(p+8,old_z);sw(angles+2,old_pitch);cc_builder_zoom_object(p);mats();return 0;
}
extern "C" uint32_t cc_builder_step(){
    int32_t amount=dwa(s(speed));uint32_t magnitude=amount<0?0u-uint32_t(amount):uint32_t(amount);int delta=w(amount);
    if((magnitude*2u)>>16)delta=amount<0?-32767:32767;
    sw(vector,delta);sw(vector+2,0);sw(vector+4,0);transform();
    for(unsigned i=0;i<3;++i)addl(ox+i*4,dwa(sar(s(surface+i*2),4)));return 0;
}
extern "C" uint32_t cc_builder_dispatch(){
    unsigned p=0,n=0,t=0;int direction=0,x=private_state[5],y=private_state[6];
    switch(phase){
    case 1:if(--scenes)return 1;wb(nomove,0);copy(save,ox,18);p=nearest();if(!p){beep();source_error(70);return 3;}sw(ptr,p);sw(type,u(p+72));sw(cursor+24,0);goto movement;
    case 2:if(--scenes)return 1;goto create;
    case 3:if(cc_flush_tiles())return 3;active=false;phase=0;return 2;
    case 4:goto move_axes;
    case 5:goto object_axes;
    default:break;
    }
    if(b(0x26c)==0x81)return finish_request();
    if(b(0x26c)==0xa2){wb(0x26c,0);sl(cursor+8,0);}
    cc_builder_quality_key();n=u(closest);
    if(b(0x26c)==0xca){wb(0x26c,0);n=uint16_t(n+1);}
    if(b(0x26c)==0xce){wb(0x26c,0);n=uint16_t(n-1);if(w(n)<0)n=0;}
    if(n!=u(closest)){sw(closest,n);goto select;}
    if(b(0x26c)==0xb9){wb(0x26c,0);sw(closest,0);goto select;}
    goto movement;
select:
    if(u(ptr)!=cursor){if(u(cursor+24)){source_error(71);beep();return 3;}sw(cursor+24,1);sw(ptr,cursor);cc_table_add(u(uf::tblist),cursor);}
    prepare_nearest();return 1;
movement:
    sw(cursor+12,raw_ticks()*32);sw(cursor+14,raw_ticks()*10);sw(cursor+16,raw_ticks()*6);
    if(key(0x1e))goto after_motion;
    phase=4;return 4;
move_axes:
    if(key(0x38)){if(cc_builder_move_cursor(x,y))return 3;}
    else if(cc_map_bump_angles(w(-y),w(-x)))return 3;
    mats();cc_builder_zoom_object(ox);if(s(ox+10)>=10)sl(ox+8,0x9ffff);
after_motion:
    cc_builder_speed_key();if(b(0x26c)==0xba){wb(0x26c,0);sw(angles+2,0);sw(angles+4,0);}
    if(key(0x4c)){sw(speed,absw(s(speed)));cc_builder_step();}
    if(key(0x52)){sw(speed,absw(s(speed)));sw(speed,-s(speed));cc_builder_step();sw(speed,absw(s(speed)));}
    if(b(0x26c)==0xaa){wb(0x26c,0);if(u(ptr)!=cursor||!u(cursor+24)){sw(ptr,cursor);show_cursor();}}
    if(b(0x26c)==0x9d){wb(0x26c,0);if(u(ptr)==cursor)sw(cursor+24,0);}
    if(b(0x26c)==0xa0){wb(0x26c,0);p=u(ptr);if(p!=cursor){sw(p+24,0);copy(p,cursor,12);sw(ptr,cursor);sw(cursor+24,1);cc_table_add(u(uf::tblist),cursor);}}
    if(b(0x26c)!=0xae)goto change_type;
    wb(0x26c,0);
    if(u(ptr)!=cursor){if(u(cursor+24)){beep();goto change_type;}sw(cursor+24,1);sw(ptr,cursor);cc_table_add(u(uf::tblist),cursor);goto create;}
    if(!u(cursor+24)){show_cursor();phase=2;scenes=2;return 1;}
create:
    if(cc_builder_copy_template(buf,u(type))){source_error(62);return 3;}
    copy(cursor,buf,12);sw(cursor+24,0);sw(ptr,promote_buffer());cc_builder_set_quality(b(qual));
change_type:
    p=u(ptr);if(p==cursor)goto rotate;
    t=u(p+72);sw(type,t);
    if(b(0x26c)==0x8d){wb(0x26c,0);t=uint16_t(t+1);direction=1;}
    if(b(0x26c)==0x8c){wb(0x26c,0);t=uint16_t(t-1);direction=-1;}
    if(!direction)goto rotate;
    for(unsigned tries=0;tries<65536;++tries){if(w(t)<0)t=u(uf::ntemplates);if(t>u(uf::ntemplates))t=0;if(t==u(type))goto rotate;if(!cc_builder_valid_type(t))break;t=uint16_t(t+direction);}
    copy(p,save,8);if(cc_builder_copy_template(p,t)){source_error(65);return 3;}copy(save,p,8);
rotate:
    if(key(0x1e)){phase=5;return 4;}
    goto done;
object_axes:
    if(bump_object(w(-clipped(y)),w(-clipped(x))))return 3;
    if(key(0x4c))for(unsigned i=0;i<3;++i){unsigned at=u(ptr)+12+i*2;sw(at,(u(at)+0x3fff)&0xc000);}
done:
    if(b(0x26c)==0xb2)return finish_request();phase=0;return 0;
}
extern "C" void cc_builder_scene_begin(uint32_t ticks){
    int dt=uint16_t(ticks)>>3;if(dt>=s(0xe51))dt=s(0xe51);if(dt<=s(0xe53))dt=s(0xe53);sw(0x1e5e,dt);if(!b(nomove))sw(0xf3f,u(angles+2));
}
extern "C" void cc_builder_near_begin(){near_table=u(uf::tblist);sw(uf::tbstrt,near_table);near_remaining=u(near_table);near_slot=near_table+4;near_object=0;near_had=near_remaining!=0;}
extern "C" uint32_t cc_builder_near_next(){
    if(near_object){if(!u(near_object+24)){if(cc_table_remove(near_table,near_object))return UINT32_MAX;sw(uf::tbstrt,near_table);}else near_slot+=2;--near_remaining;near_object=0;}
    while(near_remaining){
        near_object=u(near_slot);if(!valid(near_object))return UINT32_MAX;sw(uf::odcode,b(near_object+18));
        const bool blink=(u(near_object+24)&0x400)&&!(raw_ticks()&512);
        if(!blink&&!b(nomove))return near_object;
        if(!blink&&cc_distance_object(near_object)==2)return UINT32_MAX;
        if(!u(near_object+24)){if(cc_table_remove(near_table,near_object))return UINT32_MAX;sw(uf::tbstrt,near_table);}else near_slot+=2;
        --near_remaining;near_object=0;
    }
    if(near_had){if(cc_table_sort(near_table))return UINT32_MAX;if(u(near_table)>=2)wb(uf::odcode,b(u(near_table+2+u(near_table)*2)+18));near_had=false;}return 0;
}
extern "C" uint32_t cc_builder_scene_finish(){if(cc_cache_far_objects())return 3;cc_update_tiles();return 0;}

extern "C" {uint8_t* cc_framebuffer();void cc_draw_line(int32_t,int32_t,int32_t,int32_t,uint32_t);void cc_video_load(uint32_t);void cc_video_store(uint32_t);uint32_t cc_camera_flip_page();}
namespace {
#include "builder_glyphs.inc"
void glyph(unsigned c){
    private_state[2]=int32_t(c);
    for(unsigned line=0;line<15;++line){unsigned i=c*32+line*2;int x1=w(private_state[3]+glyph_x[i]),x2=w(private_state[3]+glyph_x[i+1]);
        int y1=w(private_state[4]-(glyph_y[i]>>1)),y2=w(private_state[4]-(glyph_y[i+1]>>1));
        sw(0x1b83,x1);sw(0x1b85,y1);sw(0x1b87,x2);sw(0x1b89,y2);
        if(glyph_x[i]!=glyph_x[i+1]||glyph_y[i]!=glyph_y[i+1]){wb(0x1b8b,private_state[1]);wb(0x8f86,15);if(y1==y2){int left=x1<x2?x1:x2,right=x1<x2?x2:x1;for(int px=left;px<=right;++px)if(px>=0&&px<640&&y1>=0&&y1<350)cc_framebuffer()[y1*640+px]=uint8_t(private_state[1]&15);}else cc_draw_line(x1,y1,x2,y2,private_state[1]);
            if(x2<x1){sw(0x1b83,x2);sw(0x1b85,y2);sw(0x1b87,x1);sw(0x1b89,y1);}}
    }
    private_state[3]=w(private_state[3]+11);
}
void number(int32_t value,unsigned x){
    char text[12],digits[11];unsigned n=0,k=0;uint32_t magnitude=value<0?0u-uint32_t(value):uint32_t(value);if(value<0)text[k++]='-';
    do{digits[n++]=char('0'+magnitude%10);magnitude/=10;}while(magnitude);while(n)text[k++]=digits[--n];text[k]=0;
    for(unsigned i=0;i<=k;++i)wb(0x1a1e + i,text[i]);
    private_state[4]=340;
    for(unsigned pass=0;pass<2;++pass){private_state[3]=int32_t(x);unsigned page=b(0xd5d)!=0;cc_video_load(page);
        for(unsigned i=0;i<k;++i)glyph(text[i]=='-'?11:unsigned(text[i]-'0'));
        cc_video_store(page);cc_camera_flip_page();}
    private_state[3]=int32_t(x);
}
}
extern "C" void cc_builder_numbers(){
    unsigned p=u(ptr);if(p==cursor&&!u(p+24))return;
    number(l(p),10);number(l(p+4),130);if(l(p+8)==private_state[0])private_state[1]=0;number(l(p+8),250);private_state[1]=15;
    if(u(p+24)&0x800)private_state[1]=13;if((u(p+24)&0x200)&&!(raw_ticks()&0x300))private_state[1]=1;
    number(int32_t(cc_builder_get_quality()+1),380);private_state[1]=15;number(u(p+72),580);
}
extern "C" void cc_builder_map_numbers(){number(l(ox),10);number(l(ox+4),130);number(l(ox+8),250);private_state[1]=12;number(u(angles),370);private_state[1]=15;}
// Actual command-line -rs plus independently selected g/i/j/k switches.
// No pilot options, ace flag, training generation or extra-plane switch.
extern "C" void cc_builder_startup_options(uint32_t controls){
    wb(0xaf0,255);wb(0xaf1,255);
    if(controls&1)wb(0xaf5,b(0xaf5)|1);if(controls&2)wb(0xae6,255);
    if(controls&4)wb(0xaf5,b(0xaf5)|128);if(controls&8)wb(0xaf6,0);
}
