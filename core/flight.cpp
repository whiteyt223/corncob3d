#include "demo_frame.hpp"
#include "edition.hpp"
#include "audio.hpp"
#include "fixed.hpp"
#include "flight_fields.hpp"
#include "flight_lifecycle.hpp"
#include "hud.hpp"
using namespace cc;
namespace f=flight_field;
extern "C" {
uint32_t cc_atn2(uint32_t,uint32_t);uint32_t cc_cos(uint32_t);uint32_t cc_vmag(int32_t,int32_t,int32_t);
void cc_angles_matrix(const uint16_t*,uint16_t*,uint32_t);
void cc_matrix_angles(const uint16_t*,uint16_t*);
void cc_rotate_plane(uint16_t*,int32_t,int32_t,int32_t);
void cc_matvmul(const uint16_t*,const uint16_t*,uint16_t*);
uint32_t cc_aircraft_refresh_frame();
}
namespace {
// Preserve the original packed data layout for state comparisons and saves.
uint8_t state[65536];
uint16_t u(unsigned p){return uint16_t(state[p]|(uint16_t(state[p+1])<<8));}
int32_t s(unsigned p){return w(u(p));}
int32_t l(unsigned p){return d(uint32_t(u(p))|(uint32_t(u(p+2))<<16));}
void sw(unsigned p,int32_t v){state[p]=uint8_t(v);state[p+1]=uint8_t(uint32_t(v)>>8);}
void sl(unsigned p,int32_t v){sw(p,v);sw(p+2,int32_t(uint32_t(v)>>16));}
bool flag(unsigned p){return state[p]!=0;}
void addl(unsigned p,int32_t v){sl(p,d(int64_t(l(p))+v));}
uint32_t random_word(){uint32_t seed=uint32_t(uint64_t(uint32_t(l(0xf722)))*0x278dde6du);sl(0xf722,d(seed));return seed>>16;}
void matrix(unsigned angles,unsigned out,bool inverse){
    uint16_t a[3],m[9];for(unsigned i=0;i<3;++i)a[i]=u(angles+2*i);
    cc_angles_matrix(a,m,inverse);for(unsigned i=0;i<9;++i)sw(out+2*i,m[i]);
}
void transform(unsigned m,unsigned v,unsigned out){
    uint16_t mat[9],vec[3],result[3];for(unsigned i=0;i<9;++i)mat[i]=u(m+2*i);
    for(unsigned i=0;i<3;++i)vec[i]=u(v+2*i);
    cc_matvmul(mat,vec,result);for(unsigned i=0;i<3;++i)sw(out+2*i,result[i]);
}
int32_t wa(int32_t v){return w(w(v)*s(f::ofrmticks)/s(f::ticksf));}
int32_t dwa(int32_t v){return rdiv(mul(v,s(f::ofrmticks)),s(f::ticksf));}
void refresh(){cc_aircraft_refresh_all();}

bool pitchlock(){
    if(uint16_t(absw(s(f::rtopv+4)))<150)return false;
    uint32_t speed=cc_vmag(s(f::rtopv),s(f::rtopv+2),s(f::rtopv+4));
    if(speed<3900){state[f::autoflag]=0;return true;}
    speed=uint32_t(clamp(int32_t(speed),3901,15500))*8;
    const int16_t table[8]={-2109,-1507,-1005,-772,-671,-571,-500,-450};
    unsigned index=speed/15591-2,remainder=speed%15591;
    const int32_t target=table[index]+(table[index+1]-table[index])*int32_t(remainder)/15591;
    const int32_t diff=clamp(w(s(f::effective_angles+2)-target),-2000,3200);
    const int32_t pair=d((uint32_t(uint16_t(diff))<<16)|(diff<0?65535:0));
    addl(f::pitdot,rdiv(pair,!flag(f::autoflag)&&flag(f::aeroflag)&&!s(f::qstald)?280:70));
    return true;
}
void controls(int32_t x,int32_t y,int32_t rudder){
    sw(f::xang,x);sw(f::yang,y+5);sw(f::rdrang,rudder);
    const int32_t v=clamp(s(f::ctopv),-300,300);sw(f::vpxp,v);
    int32_t a=w(s(f::xang)*s(f::rhotop)/s(f::rho0));
    int32_t torque=mul(mul(a*v,v)/300,s(f::thrsf));
    addl(f::thrdot,rdiv(d(int64_t(torque)-sar(d(int64_t(l(f::thrdot))*3),1)),s(f::thrmi)));
    addl(f::thrdot,cc_aircraft_damage_bias(s(f::thrdam),v));
    if(flag(f::autoflag)){int32_t correction=clamp(w(-s(f::effective_angles+4)),-25,25);addl(f::thrdot,sar(d((uint32_t(uint16_t(correction))<<16)|uint16_t(correction)),2));}
    int32_t slip=w(cc_atn2(u(f::topv),u(f::topv+2))),limited=clamp(slip,-150,150),cty=0;
    if(slip!=limited){int32_t tmp=sar(limited,4);cty=d((uint32_t(uint16_t(tmp))<<16)|uint16_t(hi(tmp)));}
    sw(f::ctydoth,hi(cty));sw(f::ctydotl,cty);
    int32_t sf=s(f::yawsf);if(flag(f::groundflag)&&u(f::topv)<=1500)sf=w(sf*4);
    int32_t yd=d(int64_t(mul(v*s(f::rdrang),sf))-l(f::yawdot)+cty+w(-s(f::sidef))*1053);
    addl(f::yawdot,rdiv(yd,s(f::yawmi)));
    addl(f::yawdot,cc_aircraft_damage_bias(s(f::yawdam),v));
    if(s(f::ctopv)>=30){int32_t q=hm(hm(s(f::sssin),s(f::sscos)),3859);addl(f::yawdot,rdiv(d(uint32_t(uint16_t(q))<<16),w(-s(f::ctopv))));}
    a=w(s(f::yang)*s(f::rhotop)/s(f::rho0));
    addl(f::pitdot,rdiv(d(int64_t(mul(a*v,s(f::pitsf)))-l(f::pitdot)),s(f::pitmi)));
    if(flag(f::aeroflag)&&!s(f::qstald))pitchlock();
    if(flag(f::autoflag)&&!pitchlock()){
        int32_t a=clamp(w(s(f::topv+4)-s(0x19e8)),-150,150);
        int32_t rate=sar(d((uint32_t(uint16_t(a))<<16)|uint16_t(a)),3);
        a=clamp(w(-s(f::rtopv+4)),-50,50);
        addl(f::pitdot,d(int64_t(rate)+w(absw(a)*a)*20));
        // dohlock compares the raw vertical word unsigned, despite ABS AX.
        if(u(f::rtopv+4)>15)state[0xe5a]=0;
        else if(!state[0xe5a]){state[0xe5a]=255;sl(0xf47,l(f::effective_pos+8));}
        else addl(f::pitdot,mul(d(int64_t(l(0xf47))-l(f::effective_pos+8)),655));
        sw(0x19e8,s(f::topv+4));
    }
    if(flag(f::groundflag)){
        uint16_t speed=uint16_t(absw(s(f::topv)));
        if(speed<=2821){a=w(s(f::effective_angles+2)-s(f::pitchmin));addl(f::pitdot,dwa(d((uint32_t(a)<<8)|uint8_t(a))));}
        else addl(f::pitdot,w(-w(speed-2821))*32);
        a=s(f::effective_angles+2);bool skip=false;
        if(a<s(f::pitchmin)){
            a=hi(l(f::pitdot));if(a<0)skip=true;
            else {sl(f::pitdot,sar(d(-int64_t(d(int64_t(l(f::pitdot))+1000))),1));a=s(f::pitdot);}
        }
        if(!skip&&a>=0&&hi(l(f::pitdot))<0)sl(f::pitdot,sar(d(-int64_t(d(int64_t(l(f::pitdot))-1000))),1));
    }else if(s(f::qstald))addl(f::pitdot,s(f::topv+4)*128);
    if(!flag(f::groundflag))sw(f::almthstald,uint32_t(u(f::rhotop))*(uint32_t(u(f::thstal))*58000>>16)/u(f::rho0));
    if(flag(f::groundflag)||s(f::qstald))state[f::stallprotflag]=0;
    else{
        uint16_t projected=uint16_t(absw(w(2*s(f::alpha)-s(f::oldalpha))));
        bool protect=flag(f::stallprotflag)?w(absw(hi(l(f::pitdot)))-s(f::mopitdoth))>=0:projected>=u(f::almthstald);
        state[f::stallprotflag]=protect;
        if(protect){cc_hud_flash_stall(random_word());sl(f::pitdot,mul(rdiv(l(f::pitdot),w(projected)),s(f::almthstald)));sw(f::mopitdoth,absw(hi(l(f::pitdot))));}
    }
    cc_aircraft_stability_noise();
    const unsigned rates[3]={f::pitdot,f::yawdot,f::thrdot};for(unsigned off:rates)sw(off+2,clamp(s(off+2),-3000,3000));
    uint16_t m[9],angles[3];for(unsigned i=0;i<9;++i)m[i]=u(f::zmat+2*i);
    cc_rotate_plane(m,wa(s(f::thrdot+2)),wa(w(-s(f::pitdot+2))),wa(s(f::yawdot+2)));
    cc_matrix_angles(m,angles);
    for(unsigned i=0;i<9;++i)sw(f::zmat+2*i,m[i]);
    for(unsigned i=0;i<3;++i)sw(cc_aircraft_angles_offset()+2*i,angles[i]);
    if(!flag(f::ejectflag)&&(!flag(f::groundflag)||flag(f::onrunwayflag)))for(unsigned i=0;i<3;++i)sw(0x5bbc+i*2,angles[i]);
    if(!flag(f::ejectflag)&&flag(f::groundflag)&&!flag(f::onrunwayflag)){
        sw(f::angles+4,s(f::angles+4)+int32_t(random_word()%201)-100);
        sw(f::angles+2,s(f::angles+2)+int32_t(random_word()%241)-120);
    }
}
void forces(){
    sw(f::thstal,flag(f::flapflag)?3882:2912);sw(f::thstl2,flag(f::flapflag)?7764:5824);
    sw(f::thrtsf,u(f::obj5dbits)&0x1000?350:450);
    transform(f::nzmat,f::rtopv,f::topv);
    for(unsigned i=0;i<3;++i){sw(f::ctopv+2*i,sar(s(f::topv+2*i),4));sl(f::dvel+4*i,s(f::nzmat+4+6*i)*s(f::grvsf));}
    sw(f::sssin,s(f::nzmat+10));sw(f::sscos,s(f::nzmat+16));
    int32_t a=w(s(f::rhotop)-w(s(f::rho0)-s(f::rho02)));uint32_t multiplier=0;
    if(a>=0){multiplier=65535;if(uint16_t(a)<=u(f::rho02))multiplier=cc_cos(uint16_t(uint32_t(uint16_t(s(f::rho02)-a))*20383/u(f::rho02)-4000));}
    sw(f::thrtmult,multiplier);
    int32_t thrustBase=w(rdiv(d(uint32_t(uint16_t(s(f::rpm)-20))*u(f::thrtsf)),s(f::rpmx)));
    if(flag(f::rocketflag)){thrustBase=wa(5);sw(0x19f4,s(0x19f4)-thrustBase);if(s(0x19f4)<0){state[f::rocketflag]=0;sw(0x19f4,0);thrustBase=0;}thrustBase=w(thrustBase+1350);}
    uint32_t thrust=uint16_t(thrustBase)*multiplier;
    addl(f::dvel,mul(rdiv(d(thrust),s(f::ctopv)>300?s(f::ctopv):300),16));
    int32_t altitude=w(uint32_t(l(f::effective_pos+8))>>8);if(altitude<0)altitude=0;
    int32_t pressure=w(s(f::rho0z)-altitude);if(pressure<0)pressure=0;
    sw(f::rhotop,uint32_t(u(f::rho0))*uint16_t(pressure)/u(f::rho0z));
    int32_t vx=s(f::ctopv),vy=s(f::ctopv+2),den=w(vx-s(f::dvspd))<0?s(f::idsf):s(f::oidsf);
    if(flag(f::flapflag))den=sar(den,2);
    sw(f::ridsf,den);
    addl(f::dvel,mul(rdiv(mul(absw(vx)*vx,sar(s(f::rhotop),4)),den),-16));
    den=w(vy-s(f::dvspd))<0?s(f::idsf):s(f::oisdsf);sw(f::risdsf,den);
    int32_t side=mul(rdiv(mul(absw(vy)*vy,sar(s(f::rhotop),4)),den),-16);addl(f::dvel+4,side);
    sw(f::sidef,side);if((side<0?0u-uint32_t(side):uint32_t(side))&32768)sw(f::sidef,side<0?-32768:32767);
    if(flag(f::groundflag)){sl(f::dvel+4,0);sw(f::topv+2,0);sw(f::sidef,0);}
    sw(f::oldalpha,s(f::alpha));sw(f::alpha,cc_atn2(u(f::topv),u(f::topv+4)));
    int32_t test=w(s(f::alpha)+s(f::thstal));den=s(f::stlfsf);sw(f::qstald,-1);
    if(test>=0&&w(test-s(f::thstl2))<0){den=s(f::lfsf);sw(f::qstald,0);cc_audio_stall(0);cc_hud_button(3,0x070e);cc_hud_button(3,0x000c);if(flag(f::flapflag))den=sar(den,1);}
    if(s(f::qstald)&&!flag(f::groundflag)){cc_audio_stall(1);cc_hud_button(3,0x0e07);cc_hud_button(3,0x0c00);}
    int32_t z=s(f::topv+4),lift=z*z;if(z>=0)lift=-lift;
    addl(f::dvel+8,mul(mul(rdiv(rdiv(lift,16),den),s(f::rhotop)),16));
    for(unsigned i=0;i<3;++i)sw(f::acctop+2*i,hi(dwa(l(f::dvel+4*i))));
    sw(f::topv,w(s(f::topv)+s(f::acctop)));
    int32_t y=s(f::topv+2);if(!s(f::acctop+2)&&uint16_t(absw(y))<=200)y=0;
    sw(f::topv+2,clamp(w(y+s(f::acctop+2)),-9500,9500));sw(f::topv+4,clamp(w(z+s(f::acctop+4)),-9600,9600));
    transform(f::nnzmat,f::acctop,f::racctop);transform(f::nnzmat,f::topv,f::rtopv);
}

}
extern "C" uint8_t* cc_flight_state(){return state;}
extern "C" void cc_flight_refresh(){refresh();}
extern "C" void cc_flight_throttle(uint32_t plus,uint32_t minus){
    if(flag(f::ejectflag)&&!state[0xf54])return;
    if(plus&&(!(u(f::obj5dbits)&0x2000)||state[0xaec])){
        if(u(f::currpmx)){state[f::freezeflag]=0;state[0xad8]=0;cc_hud_button(12,0x0107);int32_t rpm=w(s(f::rpm)+20);if(w(s(f::currpmx)-rpm)<0)rpm=w(s(f::currpmx)-3);sw(f::rpm,rpm);}
        else if((u(f::obj5dbits)&0x100)&&!flag(f::ejectflag))state[f::crshflg]=8;
    }
    if(minus){int32_t rpm=w(s(f::rpm)-20);if(w(rpm-20)<0)rpm=20;sw(f::rpm,rpm);}
}
extern "C" void cc_flight_flaps(){
    state[f::flapflag]^=255;
    if(!flag(f::groundflag))sw(f::pitdot+2,s(f::pitdot+2)+(flag(f::flapflag)?-200:200));
}
// 0: active flight/pilot state; 2: invalid arithmetic/input; 3: terminal death.
// Fatal resolution remains late: call cc_aircraft_resolve_crash AFTER throttle,
// before late walking/flaps/ejection. Crash flags do not themselves pause here.
extern "C" uint32_t cc_flight_step(int32_t x,int32_t y,int32_t rudder,uint32_t brake,uint32_t ticks){
    if(cc_lifecycle_terminal())return 3;
    if(x< -512||x>512||y< -512||y>512||rudder< -1024||rudder>1024)return 2;
    if(!ticks||ticks>50||!s(f::ticksf)||!s(f::rho0)||!s(f::rho0z)||!s(f::rho02))return 2;
    sw(f::ofrmticks,ticks);
    if(!flag(f::freezeflag)){cc_aircraft_crash_prelude();cc_pilot_prepare_matrices();matrix(cc_aircraft_angles_offset(),f::zmat,true);}
    if(flag(f::ejectflag)&&!state[0xf54]){cc_pilot_look(x,y);x=y=0;}
    if(!flag(f::freezeflag)){
        controls(x,y,rudder);forces();cc_aircraft_crash_damping();uint32_t status=cc_aircraft_gear(brake);if(status)return status;
        cc_aircraft_qcrashland();cc_aircraft_home();
    }
    for(int i=3;i>=0;--i)cc_hud_flash_damage(i);
    if(u(f::totaldamage)>=50)cc_hud_flash_eject();
    if(flag(f::ejectflag)){cc_pilot_move();cc_aircraft_move_abandoned();}
    else{
        if(!flag(f::freezeflag))for(unsigned i=0;i<3;++i){int32_t v=w(sar(s(f::racctop+2*i),1)+s(f::rtopv+2*i));addl(f::pos+4*i,sar(dwa(v),u(f::obj5dbits)&0x2000?2:4));}
        cc_aircraft_home();
    }
    cc_aircraft_common_status();if(cc_edition_is_other_worlds())return cc_aircraft_refresh_frame();refresh();return 0;
}
