#include "demo_frame.hpp"
#include "edition.hpp"
#include "joystick.hpp"
#include "enemy_callbacks.hpp"
#include "late_weapons.hpp"
#include "audio.hpp"
#include "flight_lifecycle.hpp"
#include "hud.hpp"
#include "state.hpp"
#include "flight_fields.hpp"
#include "runtime.hpp"
using namespace cc;using namespace cc::state;
namespace f=flight_field;
extern "C" {
uint32_t cc_vmag(int32_t,int32_t,int32_t);int32_t cc_scos(uint32_t);
void cc_angles_matrix(const uint16_t*,uint16_t*,uint32_t);
void cc_matvmul(const uint16_t*,const uint16_t*,uint16_t*);
uint32_t cc_table_add(uint32_t,uint32_t);
uint32_t cc_effect_flotsam(uint32_t,uint32_t);uint32_t cc_effect_shards(uint32_t,uint32_t);
}
namespace {
constexpr unsigned pilot=0xb05,ang=0xb11,plane=0x5bb0,otop=0x1da2;
constexpr unsigned invinc=0xae6,home=0xf55,standing=0xf79,jumpflag=0xf7c;
constexpr unsigned jumpvalue=0xf7d,chute=0x19ee,damp=0x19ef,term=0x19f1,swing=0x19ea;
constexpr unsigned tmb=0x1d8a,oor=0x1d84;
cc_lifecycle_hook world_hook=nullptr;
bool terminal=false;
unsigned board_pending=0;
uint32_t events[1+64*22]={};
bool clock_ok(){return s(f::ofrmticks)!=0&&s(f::ticksf)!=0;}
void copy(unsigned from,unsigned to,unsigned n){for(unsigned i=0;i<n;++i)wb(to+i,b(from+i));}
void matrix(unsigned a,unsigned out,bool inverse){uint16_t av[3],m[9];for(unsigned i=0;i<3;++i)av[i]=u(a+i*2);cc_angles_matrix(av,m,inverse);for(unsigned i=0;i<9;++i)sw(out+i*2,m[i]);}
void transform(unsigned mat,unsigned vec,unsigned out){uint16_t m[9],v[3],r[3];for(unsigned i=0;i<9;++i)m[i]=u(mat+i*2);for(unsigned i=0;i<3;++i)v[i]=u(vec+i*2);cc_matvmul(m,v,r);for(unsigned i=0;i<3;++i)sw(out+i*2,r[i]);}
int32_t hv(int32_t value,int32_t factor){return w(hi(w(value)*w(factor))*2);}
void event(unsigned kind,unsigned object,unsigned detail){
    if(events[0]>=64){source_error(164);return;}
    unsigned off=1+events[0]++*22;events[off]=kind;events[off+1]=object;events[off+2]=detail;
    for(unsigned i=0;i<19;++i){uint32_t v=0;for(unsigned j=0;j<4&&i*4+j<74;++j)v|=unsigned(b(object+i*4+j))<<(j*8);events[off+3+i]=v;}
}
void fullsize(){if(u(0x1c3e)!=0x1c26){sw(0x1c40,0);wb(0xe57,0);sw(0x1c3e,0x1c26);}event(2,pilot,0);}
void funeral(bool destroyed){
    if(world_hook&&world_hook(1,plane,destroyed))return;
    // Persistence needs the host's tile/cache implementation. Retain a complete
    // record in the event before clearing the live object; do not invent a pool
    // replacement with different allocation, quantization or collision order.
    event(1,plane,destroyed);sw(plane+24,0);
    const unsigned prototype=u(0x2cca+2*2);for(unsigned i=46;i<52;i+=2)sw(prototype+i,0);
}
void qcrash_tail(){
    if(b(invinc)){cc_aircraft_resetplane();wb(f::crshlndflag,0);return;}
    sw(f::rpm,20);if(!u(f::currpmx))return;
    cc_audio_crash_landing();wb(0xf5b2,0);sw(0x1da,-1);sw(f::currpmx,0);
}
void set5dbits(){
    if(u(f::obj5dbits)&4){qcrash_tail();return;} // original cross-routine jump
    unsigned bits=(b(home)<<1)&2;if(!bits)sw(0x1e8,u(0x1e8)+1);sw(f::obj5dbits,u(f::obj5dbits)|bits|4);
}
void board_finish(){
    // getkbit eventually returns AL=0; source AND AX,4 therefore clears the
    // crashlanding flag even when the newly boarded object carries bit4.
    wb(f::crshlndflag,0);
    if(!u(f::currpmx)){wb(0xe57,0);wb(0xf3c,(b(0xf3c)&1)?255:0);}
    sw(plane+40,0x4aa9);sw(plane+42,0x4aa9);board_pending=0;
}
void ground_death(){
    wb(f::crshflg,0);
    if(!(u(f::obj5dbits)&0x100)){sw(0x1e8,u(0x1e8)+1);sw(f::obj5dbits,u(f::obj5dbits)|0x100|((b(home)<<1)&2));}
    cc_aircraft_destroy_abandoned();sw(f::currpmx,0);
}
void ground_snap(){sw(swing,0);sw(otop+4,0);sl(pilot+8,d(int64_t(l(0x2bf))+u(f::hmin)));wb(standing,255);}
void jumpadd(){sw(jumpvalue,clamp(w(int32_t(cc_lifecycle_readkey(0x24)&65535)+s(jumpvalue)),0,1024));}
// Original dolandinggear carry, with 2 reserved for invalid hardware arithmetic.
unsigned gear_contact(bool brake){
    if(b(f::crshlndflag))return 0;
    sw(f::springforce,0);wb(f::oldgflag,b(f::groundflag));wb(f::groundflag,0);sw(f::oldrtopv4,uwa(s(f::rtopv+4)));
    int32_t height=d(int64_t(l(f::effective_pos+8))-l(f::zground));
    if(hi(height)!=0)return 0;
    int32_t compression=d(int64_t(uint16_t(s(f::bounceheight)+90))-height);if(compression<0)return 0;
    const int32_t limit=b(f::onrunwayflag)?540:270;
    if(w(compression-limit)>=0){if(!b(f::onrunwayflag))compression=w(compression*2);if(w(compression-1080)<0){wb(f::crshlndflag,255);return 0;}return 1;}
    if(!b(f::oldgflag)&&!cc_audio_ambient_active())cc_audio_landing_screech();
    wb(f::groundflag,255);sw(f::topv+2,0);transform(f::nnzmat,f::topv,f::rtopv);
    sw(f::springforce,compression);int32_t spring=uint32_t(uint16_t(compression))*266/1000;
    sw(f::rtopv+4,wa(w(uwa(s(f::rtopv+4))+spring)));
    addl(f::thrdot,d(int64_t(w(s(f::tmpsum)-s(f::tmpsum+2)))*s(f::springforce)*32));
    if(brake||u(f::rpm)<=40){
        uint32_t magnitude=cc_vmag(s(f::rtopv),s(f::rtopv+2),s(f::rtopv+4));if(!magnitude||magnitude>=65536)return 2;
        const int32_t force=wa(brake?15:6),dx=force*w(-s(f::rtopv))/int32_t(magnitude),dy=force*w(-s(f::rtopv+2))/int32_t(magnitude);
        sw(f::rtopv,s(f::rtopv)+dx);sw(f::rtopv+2,s(f::rtopv+2)+dy*2); // macro repeats brake's Y add
        if(magnitude<=uint16_t(wa(100))&&u(f::rpm)<=40){sw(f::rtopv,0);sw(f::rtopv+2,0);wb(f::freezeflag,255);}
    }
    return 0;
}
}
extern "C" void cc_lifecycle_set_hook(cc_lifecycle_hook hook){world_hook=hook;}
extern "C" void cc_lifecycle_reset(){terminal=false;board_pending=0;events[0]=0;}
extern "C" uint32_t cc_lifecycle_terminal(){return terminal;}
extern "C" uint32_t* cc_lifecycle_events(){return events;}
extern "C" void cc_lifecycle_clear_events(){events[0]=0;}
extern "C" uint32_t cc_aircraft_angles_offset(){return b(f::ejectflag)?plane+12:ang;}
extern "C" void cc_pilot_prepare_matrices(){if(b(f::ejectflag)){matrix(ang,0x1fde,false);matrix(ang,0x1ff0,true);}}
extern "C" void cc_aircraft_refresh_all(){
    unsigned a=cc_aircraft_angles_offset();matrix(a,f::nzmat,false);matrix(a,f::nnzmat,true);
    matrix(ang,0x1fde,false);matrix(ang,0x1ff0,true);
    copy(b(f::ejectflag)?plane:pilot,f::effective_pos,12);copy(a,f::effective_angles,6);
}
// Original OW normal and ejected demo branches surround the effective copy.
extern "C" uint32_t cc_aircraft_refresh_frame(){
    if(!cc_edition_is_other_worlds()){cc_aircraft_refresh_all();return 0;}
    if(b(f::ejectflag)){
        cc_aircraft_refresh_all();unsigned status=cc_demo_frame_record_tail();
        if(status!=3)return status;
        // Ejected EOF cross-jumps into the normal observer matrix/update tail.
        matrix(ang,f::nzmat,false);matrix(ang,f::nnzmat,true);
        matrix(ang,0x1fde,false);matrix(ang,0x1ff0,true);
        status=cc_demo_frame_record_tail();copy(ang,f::effective_angles,6);return status;
    }
    unsigned status=cc_demo_frame_before_refresh();if(status)return status;
    matrix(ang,f::nzmat,false);matrix(ang,f::nnzmat,true);
    matrix(ang,0x1fde,false);matrix(ang,0x1ff0,true);
    status=cc_demo_frame_record_tail();copy(pilot,f::effective_pos,18);return status;
}
extern "C" void cc_aircraft_fullsize(){fullsize();}
extern "C" void cc_aircraft_damage_bar(uint32_t kind){cc_hud_damage(kind);}
extern "C" uint32_t cc_aircraft_damage_component(uint32_t kind){
    if(!clock_ok())return 2;
    if(kind==0){sw(0x1a0b,s(0x1a0b)+1);uint32_t candidate=uint32_t(u(f::currpmx))*u(0x1a0d)/1000;if(candidate<20)return 0;if(candidate>65535)return 2;sw(f::currpmx,candidate);if(u(f::rpm)>candidate)sw(f::rpm,candidate);cc_aircraft_damage_bar(0);}
    else if(kind==1){if(uint16_t(absw(s(f::thrdam)))>1600)return 0;cc_aircraft_damage_bar(1);int32_t delta=uwa(s(0x1a03));if(w(random_word())<0)delta=w(-delta);sw(f::thrdam,s(f::thrdam)+delta);sw(0x1a01,s(0x1a01)+s(0x1a07));}
    else if(kind==2){sw(f::stabdam,s(f::stabdam)+uwa(s(0x1a09)));cc_aircraft_damage_bar(2);}
    else if(kind==3){if(uint16_t(absw(s(f::yawdam)))>300)return 0;cc_aircraft_damage_bar(3);int32_t delta=uwa(s(0x1a05)),r=w(random_word());if(s(f::yawdam))r=s(f::yawdam);if(r<0)delta=w(-delta);sw(f::yawdam,s(f::yawdam)+delta);}
    else return 2;
    return 0;
}
extern "C" uint32_t cc_aircraft_damage(uint32_t count){
    if(b(invinc)||u(f::totaldamage)>200)return 0;
    if(!clock_ok())return 2;
    unsigned n=uint16_t(count);if(!n)n=65536;
    for(unsigned i=0;i<n;++i){wb(0x1a0f,6);sw(f::totaldamage,u(f::totaldamage)+1);unsigned error=cc_aircraft_damage_component(random_bound(4));if(error)return error;}
    if(u(f::totaldamage)>u(0x230))sw(0x230,u(f::totaldamage));
    return 0;
}
extern "C" void cc_aircraft_clear_damage(){const unsigned fields[]={f::thrdam,f::stabdam,0x1a0bu,f::yawdam,0x1a01u,f::totaldamage};for(unsigned p:fields)sw(p,0);wb(0xf50,0);sw(f::currpmx,200);wb(0x2158,7);}
extern "C" int32_t cc_aircraft_damage_bias(int32_t damage,int32_t speed){damage=w(damage);int32_t pair=d((uint32_t(uint16_t(damage))<<16)|(damage<0?65535:0));return mul(sar(pair,3),speed)/300;}
extern "C" void cc_aircraft_stability_noise(){
    if(!s(f::stabdam))return;
    unsigned range=uint16_t(absw(w(s(f::stabdam)*s(f::vpxp)/300))+2);
    const unsigned rates[]={f::pitdot,f::yawdot,f::thrdot};for(unsigned p:rates)sw(p+2,s(p+2)+w(int32_t(random_bound(range))-int32_t(range>>1)));
}
extern "C" void cc_aircraft_crash_prelude(){
    if(!b(f::crshlndflag))return;
    unsigned p=b(f::ejectflag)?plane:pilot;sw(p+14,0);sw(p+16,0);sw(p+12,s(p+12)+s(0x19c0));
    if(b(f::ejectflag)){sw(p+2,0);sw(p+8,s(f::hmin));}else sl(p+8,d(int64_t(l(f::zground))+u(f::hmin)));
}
extern "C" void cc_aircraft_crash_damping(){
    if(!b(f::crshlndflag))return;
    sw(0xe43,s(0xe43)+2);sw(f::rtopv+4,0);
    sw(f::rtopv,hv(s(f::rtopv),30000));sw(f::rtopv+2,hv(s(f::rtopv+2),30000));
    sw(0x19c0,absw(s(f::rtopv))+absw(s(f::rtopv+2)));
    if(u(0x19c0)<=200){sw(0xe43,0);cc_audio_scrape_stop();wb(f::freezeflag,255);sw(f::currpmx,0);if(b(f::ejectflag))funeral(true);}
}
extern "C" void cc_aircraft_resetplane(){
    wb(f::crshflg,0);int32_t v=s(f::rtopv+4);if(v<0){int32_t a=w(-v),q=a;if(u(f::topv)>=3000)q=sar(q,2);sw(f::rtopv+4,a-sar(q,1));}
    unsigned p=b(f::ejectflag)?plane:pilot;sl(p+8,u(f::hmin));sw(p+14,0);sw(p+16,0);
}
extern "C" void cc_aircraft_qcrashland(){
    if(!b(f::crshlndflag))return;
    if(b(f::ejectflag)&&u(plane+24)){set5dbits();cc_aircraft_destroy_abandoned();}
    qcrash_tail();
}
extern "C" void cc_aircraft_destroy_abandoned(){
    sw(plane+24,1);sw(plane+10,0);for(unsigned p=46;p<52;p+=2)sw(plane+p,0);sw(plane+8,100);wb(0x1cd,0);
    cc_effect_flotsam(plane,15);cc_effect_shards(plane,15);sl(plane+8,0);wb(f::freezeflag,255);sw(f::rpm,20);funeral(true);
    // bigboom only affects sound throttle, no gameplay PRNG.
    if(uint16_t(raw_ticks()-u(0xf161))>=512){sw(0xf161,raw_ticks());cc_audio_big_boom();}
}
extern "C" uint32_t cc_aircraft_gear(uint32_t brake){
    int32_t height=d(int64_t(l(f::effective_pos+8))-l(f::zground));bool failed=false;
    if(height<0){wb(f::crshflg,4);failed=true;}
    else if(d(int64_t(height)-u(f::hlow)-u(f::hmin))>=0){if(!b(home))sw(0x1e4,u(0x1e4)&~4);wb(f::groundflag,0);return 0;}
    else{
        wb(f::crshflg,5);if(uint16_t(absw(s(f::effective_angles+4)))>10000)failed=true;
        else{wb(f::crshflg,7);if(uint16_t(absw(s(f::effective_angles+2)))>10000)failed=true;else{
            wb(f::crshflg,0);uint16_t m[9],v[3],out[3];for(unsigned i=0;i<9;++i)m[i]=u(f::nnzmat+i*2);
            for(unsigned i=0;i<2;++i){v[0]=0;v[1]=uint16_t(i?-s(f::hlow):s(f::hlow));v[2]=uint16_t(-s(f::hmin));cc_matvmul(m,v,out);sw(f::tmpsum+i*2,w(-w(out[2])));}
            sw(f::bounceheight,s(f::tmpsum)>s(f::tmpsum+2)?s(f::tmpsum):s(f::tmpsum+2));
        }}
    }
    if(failed){if(b(f::ejectflag)){ground_death();return 0;}wb(f::crshflg,b(f::crshflg)|1);}
    unsigned status=gear_contact(brake!=0);if(status==2)return 2;
    if(status){if(b(f::ejectflag))ground_death();else wb(f::crshflg,2);}return 0;
}
extern "C" void cc_aircraft_home(){unsigned h=(b(0xf57)|(b(0xf52)>>1))&(b(0xf52)&1);wb(0xf57,h);wb(home,h);if(!h&&u(0x1bc8)>=20)wb(0xf56,0);}
extern "C" void cc_aircraft_common_status(){if(b(chute)&&!(b(chute)&128)){sw(swing,508);wb(chute,255);}if(b(f::crshlndflag))sw(0x1e4,u(0x1e4)|(b(home)?0x200:0x40));}
extern "C" uint32_t cc_aircraft_resolve_crash(){
    if(terminal)return 1;
    if(!b(f::crshflg))return 0;
    if(b(invinc)){cc_aircraft_resetplane();return 0;}
    cc_audio_crash();wb(0xf5b2,0);sw(0x1da,-1);sw(0x1e4,(u(0x1e4)&0xfffe)|2);sw(0x1e8,u(0x1e8)+1);
    for(unsigned i=0;i<18;++i)random_bound(126);
    terminal=true;event(4,pilot,b(f::crshflg));return 1;
}
extern "C" void cc_aircraft_eject(){
    wb(0x26c,0);if(b(f::ejectflag))return;cc_audio_eject();wb(0xf5b2,1);sw(0x1d01,20);wb(0x1d03,1);wb(0xae4,0);wb(0xf51,0);wb(0xf50,255);
    sw(plane+24,0x11|u(f::obj5dbits));cc_table_add(u(0x2393),plane);wb(f::ejectflag,b(f::ejectflag)+1);wb(0x271,255);
    copy(pilot,plane,18);copy(ang,oor,6);
    if(b(f::crshlndflag)){
        set5dbits();sw(plane+24,(0xe1|u(f::obj5dbits))&~8);for(unsigned p=40;p<46;p+=2)sw(plane+p,0x47a1);
        sl(plane+8,d(int64_t(l(f::zground))+(u(f::hmin)>>1)));wb(f::freezeflag,255);
    }
    if(b(f::crshlndflag)||b(f::groundflag)){
        sl(0x2bf,l(f::zground));sl(pilot+8,d(int64_t(l(f::zground))+u(f::hmin)));sw(plane+24,u(plane+24)&~8);wb(0xf4f,255);addl(pilot+4,2000);
        for(unsigned i=0;i<3;++i)sw(otop+i*2,0);
        wb(standing,255);
    }else{
        if(!b(0x19ec))sw(0x1e4,u(0x1e4)|0x20);
        wb(standing,0);copy(f::rtopv,otop,6);
        sw(tmb,(random_bound(8192)&0xefff)|0x800);sw(tmb+2,(random_bound(8192)&0xefff)|0x800);
    }
    fullsize();sw(damp,32000);
}
extern "C" uint32_t cc_aircraft_board_pending(){return board_pending;}
extern "C" uint32_t cc_aircraft_board(uint32_t object){
    object=uint16_t(object);
    if(board_pending){if(object!=board_pending||cc_lifecycle_key_down(0x4c))return 2;board_finish();return 0;}
    if(!b(f::ejectflag)){source_error(77);return 1;}
    wb(0xf50,0);if(u(object+24)&0x104)sw(f::currpmx,0);else cc_aircraft_clear_damage();
    sw(0xf6d,0);sw(0xf6f,0);sw(0xf71,0);
    if(u(plane+24))funeral(u(plane+40)==0x47a1);
    sw(f::obj5dbits,u(object+24)&0x3106);copy(object,pilot,18);
    for(unsigned i=0;i<3;++i){sw(f::rtopv+i*2,0);sw(f::ctopv+i*2,0);sw(f::topv+i*2,0);}
    sw(f::pitdot+2,0);sw(f::yawdot+2,0);sw(f::thrdot+2,0);
    cc_weapon_interior_view();
    if(u(f::currpmx)){
        wb(0xf5b2,0);cc_audio_reset_missile_patch();
        cc_hud_damage_init();
    }
    wb(f::groundflag,255);wb(f::flapflag,0);sw(f::rpm,20);wb(f::autoflag,0);wb(0xe4e,3);wb(f::freezeflag,255);
    wb(0xf4f,0);wb(f::ejectflag,0);wb(standing,0);wb(chute,0);wb(0xae4,b(f::joyflag));
    if(b(f::joyflag)&&cc_lifecycle_key_down(0x4c)){board_pending=object;return 2;}
    board_finish();return 0;
}
extern "C" void cc_aircraft_pull_chute(){wb(0x26c,0);if(b(f::ejectflag)&&!b(chute))wb(chute,1);}
extern "C" void cc_pilot_damp(){
    sw(otop,hv(s(otop),s(damp)));sw(otop+2,hv(s(otop+2),s(damp)));
    int32_t numerator=dwa(s(otop+4)*50);
    if(uint16_t(absw(s(term)))<=uint16_t(absw(hi(numerator)))){sw(otop+4,0);source_error(99);}
    else{int32_t q=numerator/s(term);if(q< -32768||q>32767){source_error(99);return;}sw(otop+4,s(otop+4)-q);}
}
extern "C" void cc_pilot_move(){
    if(!b(f::ejectflag))return;
    bool force_air=false;
    if(b(standing)&&cc_lifecycle_key_down(0x24)){wb(jumpflag,255);jumpadd();}
    else if(b(jumpflag)){wb(jumpflag,0);jumpadd();int32_t j=s(jumpvalue);sw(jumpvalue,0);if(j){sw(otop+4,j*4);wb(chute,0);sw(damp,31000);sw(term,4800);force_air=true;}}
    if(!force_air){if(cc_lifecycle_key_down(0x12)){sl(0x2bf,0);force_air=true;}else if(d(int64_t(l(pilot+8))-720-l(0x2bf))<=0){ground_snap();return;}}
    wb(standing,0);for(unsigned i=0;i<3;++i)addl(pilot+i*4,dwa(sar(s(otop+i*2),4)));
    if(b(chute)){sw(damp,30000);sw(term,900);}sw(otop+4,s(otop+4)-wa(50));cc_pilot_damp();
    if(d(int64_t(l(pilot+8))-l(0x2bf)-u(f::hmin))>=0)return;
    unsigned impact=uint16_t(-s(otop+4));if(impact<1800||(!b(chute)&&impact<3200)||b(invinc)){ground_snap();return;}wb(f::crshflg,3);
}
extern "C" void cc_pilot_swing(int32_t second_x,int32_t second_y){
    if(uint16_t(absw(w(second_x))+absw(w(second_y)))>32){sw(swing,0);return;}
    unsigned count=u(swing);if(count==500){sw(tmb,0);sw(tmb+2,0);copy(oor,ang,6);}
    else if(count<500){int32_t a=w(count-250);if(a<0)a=0;int32_t n=hv(a,cc_scos(uint16_t(a*900)));sw(ang+2,s(ang+2)+n*2);sw(ang+2,s(ang+2)+sar(w(-s(ang+2)),5));}
    sw(swing,count-1);
}
extern "C" void cc_pilot_look(int32_t x,int32_t y){
    if(!b(f::ejectflag)||b(0xf54))return;
    sw(tmb,s(tmb)*9500/10100);sw(tmb+2,s(tmb+2)*9500/10100);
    sw(ang+2,s(ang+2)+w(wa(sar(w(-y+s(tmb+2)),1))*2));sw(ang,s(ang)+w(wa(sar(w(-x),1))*2));
    sw(ang+4,s(ang+4)+wa(sar(w(-s(ang+4)),6)));
    if(u(swing)){uint32_t inputs=b(0xaf6)?cc_joystick_read_centered():cc_lifecycle_arrows();cc_pilot_swing(w(inputs),w(inputs>>16));}
}
extern "C" void cc_aircraft_move_abandoned(){
    if(!b(f::ejectflag)||s(plane+10)<0||b(f::freezeflag)||!u(f::currpmx))return;
    for(unsigned i=0;i<3;++i)addl(plane+i*4,dwa(sar(s(f::rtopv+i*2),4)));
    if(s(plane+10)<0)cc_aircraft_destroy_abandoned();
}
extern "C" void cc_pilot_step(uint32_t bypass){
    if(!bypass){if(absw(w(raw_ticks()-u(0xe4a)))<=40)return;sw(0xe4a,raw_ticks());sw(ang,s(ang)+s(0xe48));sw(0xe48,-s(0xe48));}
    int32_t saved=s(ang+2);sw(ang+2,0);matrix(ang,0x1d6c,true);sw(ang+2,saved);
    sw(0x1d06,s(0xf6b));sw(0x1d08,0);sw(0x1d0a,0);
    uint16_t v[3]={u(0xf6b),0,0},m[9],r[3];for(unsigned i=0;i<9;++i)m[i]=u(0x1d6c+i*2);cc_matvmul(m,v,r);
    sw(0x1da8,r[0]);sw(0x1daa,r[1]);sw(0x1dac,0);
    for(unsigned i=0;i<3;++i)addl(pilot+i*4,dwa(sar(s(0x1da8+i*2),4)));
    for(unsigned i=0;i<3;++i)sw(0x1da8+i*2,0);
}
extern "C" void cc_pilot_walk(uint32_t forward,uint32_t backward){if(!b(f::ejectflag))return;if(forward){sw(0xf6b,wa(800));cc_pilot_step(0);}if(backward){sw(0xf6b,wa(-800));cc_pilot_step(0);}}
extern "C" uint32_t cc_aircraft_try_reenter(){
    if(!b(f::ejectflag)||b(0x19ec)||!u(plane+24))return 0;
    uint32_t distance=cc_enemy_pilot_distance(plane);if(distance>65535)return 0;
    if(distance>4000){wb(0xf51,255);return 0;}if(!b(0xf51)||distance>2000)return 0;
    cc_weapon_interior_view();wb(0xf50,u(f::totaldamage)?255:0);cc_audio_reset_missile_patch();wb(0xf5b2,0);cc_hud_damage_init();cc_hud_redraw_buttons();copy(plane,pilot,18);sw(plane+24,0);wb(f::ejectflag,0);wb(0xf4f,0);return 1;
}
extern "C" void cc_lifecycle_key_event(uint32_t raw,uint32_t tick){
    unsigned scan=uint8_t(raw),key=scan&127,p=0x2d7+key*8;wb(0x26c,scan);wb(0x26d,255);
    if(!(scan&128)){if(!u(p)){sw(p+2,tick);sw(p,65535);}}
    else if(u(p)){sw(p+4,u(p+4)+uint16_t(tick-u(p+2)));sw(p,0);}
    unsigned off=0x2c7+(key>>3),mask=1u<<((-key)&7);wb(off,(b(off)&~mask)|((scan&128)?0:mask));
}
extern "C" uint32_t cc_lifecycle_key_down(uint32_t scan){unsigned key=scan&127;return (b(0x2c7+(key>>3))>>((-key)&7))&1;}
extern "C" uint32_t cc_lifecycle_readkey(uint32_t scan){
    unsigned p=0x2d7+(scan&255)*8,held=u(p+4);if(u(p)){held=uint16_t(held+uint16_t(raw_ticks()-u(p+2)));sw(p+2,raw_ticks());}
    sw(p+4,0);unsigned elapsed=uint16_t(raw_ticks()-u(p+6));sw(p+6,raw_ticks());return uint16_t(held)|(uint32_t(elapsed)<<16);
}
extern "C" uint32_t cc_lifecycle_scalekey(uint32_t scan,uint32_t scale){uint32_t r=cc_lifecycle_readkey(scan);unsigned elapsed=r>>16;if(w(elapsed)<=0)return 0;scale&=65535;uint32_t product=(r&65535)*scale;return product/elapsed>scale?scale:product/elapsed;}
extern "C" uint32_t cc_lifecycle_arrows(){if(b(f::desireflag))return 0;int32_t left=cc_lifecycle_scalekey(0x4b,512),right=cc_lifecycle_scalekey(0x4d,512),up=cc_lifecycle_scalekey(0x48,512),down=cc_lifecycle_scalekey(0x50,512);return uint16_t(right-left)|(uint32_t(uint16_t(down-up))<<16);}
