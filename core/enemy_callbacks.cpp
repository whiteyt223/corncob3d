#include "edition.hpp"
#include "late_weapons.hpp"
#include "audio.hpp"
#include "enemy_callbacks.hpp"
#include "state.hpp"
#include "runtime.hpp"
using namespace cc;
using namespace cc::state;
extern "C" {
uint32_t cc_atn2(uint32_t,uint32_t); uint32_t cc_vmag(int32_t,int32_t,int32_t);
uint32_t cc_dsqrt(uint32_t); int32_t cc_ssin(uint32_t); int32_t cc_scos(uint32_t);uint32_t cc_sin(uint32_t);
void cc_calcmat(const uint16_t*,uint16_t*); void cc_ncalcmat(const uint16_t*,uint16_t*);
void cc_matvmul(const uint16_t*,const uint16_t*,uint16_t*);
uint32_t cc_new_effect(uint32_t); uint32_t cc_effect_boom(uint32_t);
uint32_t cc_effect_flotsam(uint32_t,uint32_t); void cc_complete_objective(uint32_t);
uint32_t cc_effect_shards(uint32_t,uint32_t);
uint32_t cc_combat_hit(uint32_t,uint32_t);
uint32_t cc_aircraft_board(uint32_t);
void cc_aircraft_destroy_abandoned();
void cc_aircraft_clear_damage();
uint32_t cc_aircraft_damage(uint32_t);
uint32_t cc_world_funeral(uint32_t);
}
namespace {
bool invalid=false;
uint32_t radar_event[5]={};
uint32_t palette_event[65]={};
unsigned relative_mask=0;
int32_t sine(unsigned a){wb(0xf378,cc_sin(a)>>16);return cc_ssin(a);}
int32_t cosine(unsigned a){return sine(uint16_t(a+16384));}
bool valid_object(unsigned p){return p<=65536-74;}
bool valid_time(){return s(0x1b7f)>0&&s(0x1e5e)>0;}
void inc(unsigned p){sw(p,u(p)+1);}
unsigned quality(unsigned p){return (u(p+24)>>12)&7;}
unsigned table(unsigned p,unsigned t){unsigned row=uint16_t(t+2+uint8_t(u(t)*2)*quality(p));sw(0xec32,row);return row;}
void copy(unsigned from,unsigned to,unsigned count){for(unsigned i=0;i<count;++i)wb(to+i,b(from+i));}
void zero_velocity(unsigned p){for(unsigned i=0;i<3;++i)sw(p+46+2*i,0);}
uint32_t absolute32(int32_t value){return value<0?uint32_t(-int64_t(value)):uint32_t(value);}
unsigned magnitude(unsigned p){unsigned v=cc_vmag(s(p),s(p+2),s(p+4));if(v>=65536)invalid=true;return uint16_t(v);}
unsigned angle(int32_t x,int32_t y){unsigned a=cc_atn2(uint16_t(x),uint16_t(y));if(a>=65536)invalid=true;return uint16_t(a);}
int32_t divide(int32_t n,int32_t divisor){
    if(!divisor||(n==INT32_MIN&&divisor==-1)){invalid=true;return 0;}
    int32_t q=n/divisor;if(q<-32768||q>32767)invalid=true;return w(q);
}
// rand_w's final ADD carry is observable by the bee tail-call.
unsigned random_carry(bool& carry){
    uint32_t seed=uint32_t(l(0xf722)),m=uint32_t(l(0xf726));
    uint32_t lo=seed&65535,hi=seed>>16,ml=m&65535,mh=m>>16;
    unsigned mid=uint16_t(uint16_t(mh*lo)+((ml*lo)>>16));
    carry=unsigned(uint16_t(ml*hi))+mid>65535;
    seed=uint32_t(uint64_t(seed)*m);sl(0xf722,d(seed));return seed>>16;
}
unsigned random(){bool carry;return random_carry(carry);}
unsigned bound(unsigned n){unsigned v=random();return uint16_t(n)?v%uint16_t(n):v;}
// rel3d 6df7 / relobj5d 6d39. The abandoned aircraft branch has no z0 scaling.
bool relative(unsigned p,unsigned target){
    relative_mask=0;
    for(unsigned i=0;i<3;++i){int32_t v=d(int64_t(l(p+4*i))-l(target+4*i));sl(0xe7e + 4*i,v);relative_mask|=(absolute32(v)*2u)>>16;}
    unsigned below=uint16_t(-s(0xe88)),z0=u(0x1efa);
    if(target==0xb05&&z0&&!(below&32768)&&below>z0){
        unsigned height=uint16_t(absolute32(l(0xe86))>>12),limit=uint16_t(z0<<4);
        if(height>limit){
            unsigned factor=((limit<<16)/height)>>1;sw(0x1efc,factor);relative_mask=0;
            for(unsigned i=0;i<3;++i){
                int64_t product=int64_t(l(0xe7e + 4*i))*w(factor);uint64_t bits=uint64_t(product);
                for(unsigned j=0;j<8;++j)wb(0x20c8 + j,unsigned(bits>>(8*j)));
                sl(0xe7e + 4*i,d(uint32_t((bits>>16)<<1)));
                relative_mask|=absw(w(bits>>32));
            }
        }
    }
    return !(relative_mask&0xfff0);
}
bool observer(unsigned p,bool force){
    if(!relative(p,!force&&b(0xafc)?0x5bb0:0xb05))return false;
    for(unsigned i=0;i<3;++i)sw(0x1dce + 2*i,-sar(l(0xe7e + 4*i),6));
    sw(0xf102,magnitude(0x1dce));return true;
}
unsigned yaw(unsigned vector=0x1dce){return uint16_t(angle(-s(vector),-s(vector+2))+32768);}
unsigned pitch(unsigned vector=0x1dce){int32_t z=s(vector+4);sw(vector+4,0);return angle(magnitude(vector),-z);}
int32_t turn(unsigned p,unsigned wanted,int32_t limit){
    int32_t error=sar(w(wanted-u(p)),4);sw(p,u(p)+wa(clamp(error,-limit,limit)));return error;
}
void matrices(unsigned p,bool forward){
    uint16_t trig[6],out[9];
    for(unsigned i=0;i<3;++i){trig[2*i]=uint16_t(sine(u(p+12+2*i)));trig[2*i+1]=uint16_t(cosine(u(p+12+2*i)));}
    if(forward){cc_calcmat(trig,out);for(unsigned i=0;i<9;++i)sw(0x1d5a + 2*i,out[i]);}
    cc_ncalcmat(trig,out);for(unsigned i=0;i<9;++i)sw(0x1d6c + 2*i,out[i]);
    // ncalcmat negsin mutates original shared sine values and leaves cosines.
    for(unsigned i=0;i<6;++i)sw(0x20ae + 2*i,(i&1)?trig[i]:-w(trig[i]));
}
void velocity(unsigned p,unsigned speed){
    uint16_t mat[9],vec[3]={uint16_t(speed),0,0},out[3];
    for(unsigned i=0;i<9;++i)mat[i]=u(0x1d6c + 2*i);
    for(unsigned i=0;i<3;++i)sw(0x1dce + 2*i,vec[i]);
    cc_matvmul(mat,vec,out);for(unsigned i=0;i<3;++i)sw(p+46+2*i,out[i]);
}
void progress(unsigned p,unsigned accumulator){unsigned q=quality(p);if(q>=4&&!b(0xae9)&&!b(0xafc))addl(accumulator,dwa(q*q));}
void aa(unsigned p,bool saucer=false){ // aatim a44c
    wb(0xec3b,saucer?255:0);inc(0xf14e);unsigned row=table(p,0xed61);inc(0xf13e + 2*quality(p));wb(0x3b50,u(row));
    if(!observer(p,false)){zero_velocity(p);return;}
    if(u(0xf102)<=3000)progress(p,0xf12c);
    if(u(row+10))for(unsigned i=0;i<3;++i){
        int32_t prod=d(int64_t(s(0xf102))*sar(s(0x1dc2 + 2*i),4));
        if(uint16_t(uint16_t(absw(hi(prod)))*2)>=u(0xf0f0))break;
        sw(0x1dce + 2*i,s(0x1dce + 2*i)+divide(prod,s(0xf0f0)));
    }
    if(saucer){sw(p+14,u(p+14)+pitch());sw(p+12,yaw());}
    else {turn(p+14,pitch(),s(row+8));turn(p+12,yaw(),s(row+6));}
    if(!u(p+10)&&u(p+8)<=2000&&s(p+14)>=-1200)sw(p+14,-1200);
    matrices(p,true);if(random()>=u(row+2))return;
    sw(0x1dce,u(0xf0f0));sw(0x1dd0,0);sw(0x1dd2,0);
    unsigned child=cc_new_effect(22);if(!child)return;
    wb(child+18,0);sw(child+52,uwa(150));sw(child+24,u(child+24)|0x8000);copy(p,child,12);addl(child+8,1600);
    velocity(child,u(0xf0f0));sw(child+60,u(row+4));
}
bool radar(unsigned p){
    if(!relative(p,b(0xafc)?0x5bb0:0xb05))return false;
    progress(p,0xf128);
    for(unsigned i=0;i<2;++i)sw(0xf0d4 + 2*i,-w(uint32_t(l(0xe7e + 4*i))>>4));
    unsigned a=angle(-s(0xf0d4),-s(0xf0d6));sw(0xf0d8,0);sw(0xf102,magnitude(0xf0d4));
    unsigned radius=cc_dsqrt(u(0xf102));if(radius>=65536)invalid=true;sw(0xf0cb,radius);
    ++radar_event[0];radar_event[1]=p;radar_event[2]=uint16_t(radius);radar_event[3]=uint16_t(a-u(0xe97));radar_event[4]=b(0x1b8b);
    sw(0xf0d8,-w(uint32_t(l(0xe86))>>4));return true;
}
void kla(unsigned p){ // dballtim a9d2
    sw(0xf0ce,64);inc(0xf13a);unsigned row=table(p,0xec6d);inc(0xf115 + 2*quality(p));wb(0x2dc9,u(row));
    if(u(p+52)>=23900)return;
    wb(p+64,0);sw(p+60,0);sw(p+62,0);sw(0xf104,u(row+12));wb(0x1b8b,10);
    if(!radar(p)){zero_velocity(p);if(u(p+52)<=11950)sw(p+52,1);return;}
    if((u(0xf102)>>6)<=u(0xf0ce)){sw(0xf0ce,u(0xf102)>>6);wb(0xf0d0,quality(p));}
    int32_t pe=turn(p+14,pitch(0xf0d4),s(row+6));
    int32_t ye=turn(p+12,yaw(0xf0d4),s(row+4));sw(0xf10a,absw(pe)>absw(ye)?absw(pe):absw(ye));
    sw(p+16,u(p+16)+wa(s(row+22)));matrices(p,false);sw(0xf0ec,magnitude(p+46));
    if(u(0xf10a)>u(row+18))sw(0xf104,u(row+14));
    else{if(raw_ticks()&512)wb(0x2dc9,u(row+2));if(u(0xf102)>=200)sw(0xf104,u(row+10));else if(b(0xf127))sw(0xf104,u(row+14));}
    int32_t correction=divide(w(u(0xf104)-u(0xf0ec)),s(row+8));
    sw(0xf0ec,u(0xf0ec)+correction+w(bound(u(row+16))-(u(row+16)>>1)));velocity(p,u(0xf0ec));
    if(!u(p+10)&&u(p+8)<=1000&&s(p+50)<0)sw(p+50,0);
}
void site(unsigned p,bool vehicle){
    inc(vehicle?0xec42:0xec40);unsigned row=table(p,vehicle?0xee6b:0xed2f);
    wb(vehicle?0x601a:0x72ee,u(row));sw(vehicle?0xf0f2:0xec34,u(row+(vehicle?2:4)));
    if(u(p+52)>=(vehicle?u(0xf0f2):100))return;
    unsigned pop=vehicle?0xf0e8:0x1dc,lim=vehicle?0xf0ea:0xf113;
    if(vehicle){unsigned twice=uint16_t(u(0xf0f2)*2);sw(p+52,twice+(twice>>1));if(u(pop)>=(u(lim)>4?u(lim):4))return;}
    else {if(u(pop)>=u(lim)){inc(p+52);return;}sw(p+52,u(0xec34));}
    sw(0xec30,quality(p));inc(pop);sw(0x8fa2,p);unsigned child=cc_new_effect(vehicle?12:24);
    if(child){
        wb(child+18,0);sw(child+52,vehicle?uwa(s(0xf0f4)):24000);sw(child+24,u(child+24)|0x8000|((u(0xec30)&7)<<12));
        zero_velocity(child);sw(child+12,0);sw(child+14,vehicle?0:-16384);sw(child+16,0);copy(p,child,12);
    }
    // Failure deliberately writes the mutable template pointed to by expobj.
    if(vehicle){unsigned cls=uint8_t(-int(u(pop)+1));wb(u(0xec28)+19,cls);wb(p+19,cls);}
}
void follow(unsigned p,unsigned row){
    matrices(p,false);unsigned speed=u(row+4);
    if(!b(p+18)){speed=u(row+6);if(u(p+24)&2){speed>>=2;if(u(p+24)&4)speed=0;}}
    velocity(p,speed);
}
void reset_vehicle(unsigned p,unsigned row){
    if(!(u(p+24)&4)){sw(p+14,0);sw(p+16,0);sw(p+12,random());}
    if(b(p+18)==3){sw(p+24,0);return;}
    bool track=!b(p+18);if(b(p+18))track=random()<=u(row+(b(p+18)==1?10:8));
    if(track){
        unsigned ejected=b(0xafc);wb(0xafc,0);bool near=observer(p,false);wb(0xafc,ejected);
        if(near){
            if(!(u(p+24)&4))sw(p+12,yaw());
            if(u(0xf102)<=256&&ejected){
                if(!b(0x8f94)){wb(0x8f94,255);sw(p+24,u(p+24)|2);}
                if(u(0xf102)<=100){wb(0x8f94,1);sw(p+24,u(p+24)|4);if(b(p+19)==1){wb(0x8f94,0);if(u(0xf102)<=32)wb(0x8f95,255);}}
                copy(p,0x8f96,12);return;
            }
        }
    }
    sw(p+24,u(p+24)&0xfff9);follow(p,row);
}
void vehicle(unsigned p){
    unsigned row=table(p,0xee8d);wb(0x601a,u(row));wb(0x601c,u(row+2));
    if(b(p+24)==0x71){if(!u(p+52))sw(p+24,0);return;}
    sl(p+8,0);if(b(p+19)==1)inc(0xec44);wb(0x601e,0);
    if(u(p+24)&0x800){if(raw_ticks()&512)wb(0x601e,15);inc(p+52);}inc(0xf13c);
    unsigned threshold=uint16_t(wa(s(row+12)));bool forced=!b(p+18)&&(u(p+24)&4);if(!b(p+18))threshold=uint16_t(threshold<<2);
    if(forced||random()<=threshold)reset_vehicle(p,row);
    if(u(p+24)&4){zero_velocity(p);if(b(p+19)!=1)follow(p,row);return;}
    sw(p+12,u(p+12)+divide(sine(uint16_t(raw_ticks()*u(row+14))),s(row+16)));follow(p,row);
}
void balloon(unsigned p,bool carry){
    sw(p+52,32000);if(carry)return;unsigned row=table(p,0xef1f);wb(0x7652,u(row));
    if(!u(p+50)||l(p+8)<16383){sw(p+50,wa(s(row+2)));return;}
    if(s(p+10)>0){sw(p+50,-wa(s(row+2)));return;}
    if(random()<=uint16_t(wa(s(row+4))))sw(p+50,-s(p+50));
}
void mortar(unsigned p){
    if(u(p+52)>9994)return;
    sw(p+52,9994);if(b(p+18))return;
    uint32_t distance=absolute32(d(int64_t(l(p))-l(0xe8b)))+absolute32(d(int64_t(l(p+4))-l(0xe8f)));
    if(distance>uint32_t(l(0xec37)))return;
    sw(p+52,10000);wb(0xec36,3);wb(0x1cd,0);if(cc_effect_flotsam(p,2)==2)invalid=true;wb(0xec36,0);
}
void aa_shell(unsigned p,bool forced=false){
    bool boom=forced||u(p+52)==1;
    if(!boom){
        if(!u(p+10)){int32_t half=sar(s(p+8),1);if(half>=0&&uint16_t(half)<=u(p+66))return;}
        uint32_t dz=absolute32(d(int64_t(l(p+8))-l(0xe93)));
        boom=dz<=4500&&random()<=uint16_t(wa(3000));
    }
    if(boom){wb(0xf15e,0x20);if(cc_effect_boom(p)==2)invalid=true;wb(0x1cd,1);if(cc_effect_flotsam(p,9)==2)invalid=true;sw(p+24,0);}
    else for(unsigned i=66;i<=70;i+=2)sw(p+i,6000);
}
void shell(unsigned p){
    unsigned row=table(p,0xefb5);if(quality(p)==6)inc(0xec3c);wb(0x677b,0);
    if(u(p+52)&0x4000){if(uint16_t(32000-u(p+52))>uint16_t(uwa(30)))sw(p+52,16000);}
    else{wb(0x677b,u(row+4));unsigned time=uint16_t(uwa(5));if(time<2)time=2;if(uint16_t(16000-u(p+52))>time)sw(p+52,32000);}
}
void attract(unsigned p){
    if(b(0xafc))return;
    observer(p,false);unsigned dist=u(0xf102);if(dist>5000)return;
    unsigned divisor=dist*dist/5000;if(divisor<10)divisor=10;
    for(unsigned i=0;i<3;++i)sw(0x1dc2 + 2*i,s(0x1dc2 + 2*i)+wa(divide(sar(l(0xe7e + 4*i),3),divisor)));
}
unsigned hq_common(unsigned p){
    sw(p+52,32000);unsigned row=table(p,0xef93);wb(0x31b7,u(row));wb(0x31b8,0);
    if((u(p+24)&0x800)&&(raw_ticks()&512))wb(0x31b8,15);
    if((u(p+24)&0x200)&&!(raw_ticks()&0x300))wb(0x31b7,b(0x31b7)^8);
    if(b(p+18)&&uint32_t(l(p+20))>uint32_t(l(p+32)))return 0;
    bool complete=false;
    switch(u(row+2)){
    case 0xbb3f:complete=u(p+72)!=0;break;
    case 0xbb49:
        wb(p+27,65);if(u(p+72)){if(!cc_edition_is_deluxe()){sw(0x1e4,u(0x1e4)|0x8000);if(!(u(0x1e4)&0x40))sw(0x1e2,u(0x1e2)&~0x40);}inc(0xf0fc);complete=true;}break;
    case 0xbb6e:complete=uint16_t(u(0xec63)+u(0xec3e))==0;break;
    case 0xbb7b:complete=u(0xec5b)&&!u(0xec57);break;
    case 0xbb8d:complete=!u(0xec61);break;
    case 0xbb98:complete=uint16_t(u(0xec55)+u(0xec51))==0;break;
    case 0xbba5:complete=uint16_t(u(0xf113)+u(0xf150))==0;break;
    case 0xbbb2:complete=uint16_t(u(0xf0ea)+u(0xf0e8))==0;break;
    default:return 1;
    }
    if(complete)cc_complete_objective(p);
    return 0;
}
unsigned hq(unsigned p,bool dead){
    if(dead){
        sw(p+52,32000);inc(0xf0e0);
        if(u(p+24)&0x200){
            if(u(p+10)||u(p+8)>11200){sw(p+54,bound(60)+20);sw(p+56,bound(40)+10);sw(p+58,bound(110)+30);wb(p+64,35);}
            else{sl(p+8,3100);zero_velocity(p);sw(p+60,0);sw(p+62,0);}
        }
        wb(0x8f89,b(0x8f89)&~2);
    }else{
        inc(0xf0e4);wb(0x8f89,b(0x8f89)&~2);sw(0x8f8a,0);sw(0xf0d1,63);
        if(observer(p,true)&&u(0xf102)<=2000){
            wb(0x8f89,b(0x8f89)|2);unsigned scaled=u(0xf102)*70/2000;sw(0x8f8a,71-scaled>35?35:71-scaled);sw(0xf0d1,scaled>63?63:scaled);
            if(u(p+24)&0x800)attract(p);
        }
    }
    return hq_common(p);
}
void home_detect(unsigned p){
    wb(0xf52,0);
    if(observer(p,true)&&u(0xf102)<=5200){
        wb(0xf52,1);if(u(0xf102)<=4200){wb(0x2dd0,0);wb(0xf52,255);return;}
    }
    wb(0x2dd0,255);sw(0x2dce,0);
}
void tower(unsigned p){
    home_detect(p);sl(0x2a9,d(int64_t(l(p))+3001-131072));sl(0x2ad,d(int64_t(l(p+4))-16000));
    // Preserve the original high-word-only universe subtraction and SHR.
    unsigned tx=uint16_t(uint16_t(u(p+2)+11+(u(p)==65535?1:0)-u(0x1f9b))>>6)-1;
    unsigned ty=uint16_t(uint16_t(u(p+6)-u(0x1f9d)-(u(p+4)<16000?1:0))>>6)-1;
    sw(0x1cb,ty*3+tx);
}
void road(unsigned p){
    sw(p+52,32000);if(u(p+24)&0x7000){sw(p+40,0x5e2a);sw(p+42,0x5e2a);sw(p+44,0x5ff2);}
}
bool enough_hits(unsigned p,unsigned other){
    if(other==65535){wb(p+27,0);return true;}
    unsigned dmg=b(other+24)&0x80?25:b(other+24)&0x40?6:1;
    unsigned hp=uint8_t(b(p+27)-dmg);
    if(!hp||(hp&128)){wb(p+27,0);return true;}wb(p+27,hp);return false;
}
void boom_sound(){
    unsigned hold=u(0x1da);if(hold==65535||(hold&&uint16_t(raw_ticks()-hold)<512))return;
    if(hold){cc_audio_preset(2);sw(0x1da,0);}
    uint16_t elapsed=uint16_t(raw_ticks()-u(0xf15f));if(elapsed>=512){sw(0xf15f,raw_ticks());cc_audio_explosion(elapsed);}
}
void boom_effect(unsigned p,unsigned deadly){wb(0xf15e,deadly);if(cc_effect_boom(p)==2)invalid=true;}
void debris(unsigned p,unsigned count,bool omni,bool shards=false){
    wb(0x1cd,omni);unsigned status=shards?cc_effect_shards(p,count):cc_effect_flotsam(p,count);if(status==2)invalid=true;
}
void friendly_hit(unsigned p,unsigned other){
    if(!enough_hits(p,other))return;
    if(other==65535||(!b(other+19)&&(b(other+72)==30||b(other+72)==21||(u(other+72)==20&&!(u(other+24)&2)))))inc(0x22e);
    sl(p+8,3100);sw(p+12,random());sw(p+40,0x406a);sw(p+42,0x4132);sw(p+44,0x4132);sw(p+24,0x11);sw(p+72,33);
    debris(p,12,false);debris(p,5,false,true);
}
void vehicle_hit(unsigned p,unsigned other){
    unsigned row=table(p,0xee8d);wb(0x601a,u(row));wb(0x601c,u(row+2));
    if(b(p+24)==0x71){if(!u(p+52))sw(p+24,0);return;}sl(p+8,0);
    unsigned other_type=u(other+72);
    if(((u(p+24)&0x800)&&other_type!=30&&other_type!=21&&other_type!=19)||!enough_hits(p,other)){
        row=table(p,0xee8d);reset_vehicle(p,row);return;
    }
    wb(p+27,255);boom_effect(p,0);inc(0x1b81);table(p,0xee8d);inc(0x20c + 2*quality(p));debris(p,9,true);
    sl(p+8,428);sw(p+16,20000);sw(p+14,2400);unsigned life=uint16_t(uwa(800));sw(p+52,life);wb(p+24,0x71);
    zero_velocity(p);for(unsigned at=66;at<=70;at+=2)sw(p+at,0);if(life)sw(p+52,(life>>2)+300);
}
bool pilot_distance(unsigned p,unsigned& distance){ // evaldist 947e, unscaled
    relative(p,0xb05);if(relative_mask)return false;
    for(unsigned i=0;i<3;++i)sw(0x71c9 + 2*i,u(0xe7e + 4*i));
    distance=magnitude(0x71c9);return true;
}
unsigned parked_plane(unsigned p){ // obj5datim 94d6
    if(u(p+24)&0x100){sw(p+40,0x47a1);sw(p+42,0x47a1);}
    wb(0x44ce,u(p+24)&0x1000?6:1);sw(p+52,32000);
    bool new_pilot=false;
    if(!b(0x19ec)){
        if(b(0x2dd0))wb(p+65,0);
        else if(!(b(p+65)&0x10)&&!(u(p+24)&0x2100)&&((u(p+24)&6)==0||(u(p+24)&6)==6)){
            sw(p+24,u(p+24)&0xfef9);copy(0x2b1,p+8,10);
            if(!(b(p+65)&1)){
                copy(0x2a9,p,16);addl(p,-int32_t(7200u*u(0x2dce)));sw(p+12,-8000);inc(0x2dce);wb(p+65,b(p+65)|1);
            }else new_pilot=b(0xafc)&&b(0xf4c);
        }
    }
    unsigned distance=0;
    if(new_pilot&&pilot_distance(p,distance)){
        wb(0xf4c,0);addl(p+4,-8000);sw(p+12,0);
        unsigned status=cc_aircraft_board(p);if(!status)sw(p+24,0);return status;
    }
    if(b(0xafc)&&!b(0x19ec)&&pilot_distance(p,distance)&&distance<=2000){unsigned status=cc_aircraft_board(p);if(!status)sw(p+24,0);return status;}
    if(u(p+24)&0x2000){sw(p+40,0x6838);sw(p+42,0x6838);}
    return 0;
}
void shard(unsigned p){
    unsigned color=0;
    if(u(p+8)&&!(divide(w(p-0x8fa6),74)&3))color=12+2*bound(2);
    sw(0x71cf,color);
}
void distort_palette(){ // distcolors 62e9 / workonclr 62d2; exact 6-bit DAC values.
    ++palette_event[0];
    for(unsigned i=0;i<16;++i){
        unsigned src=0x1e6e + 4*i;palette_event[1+4*i]=b(src);
        for(unsigned j=0;j<3;++j){
            unsigned value=uint8_t(b(src+1+j)+b(0x1eef + 2*j));value=value&64?63-(value&63):value&63;
            wb(src+65+j,value);palette_event[2+4*i+j]=value;
        }
    }
}
void distortion(unsigned p){
    inc(p+52);inc(0xec5d);
    if(!observer(p,true)||u(0xf102)>1000){wb(0x8f89,b(0x8f89)&254);return;}
    sw(p+24,u(p+24)|4);unsigned distance=1000-u(0xf102);sw(0x2dd1,distance*distance/31);wb(0x8f89,b(0x8f89)|1);
}
void distortion_hit(unsigned p,unsigned other){
    unsigned type=u(other+72);if(type!=20&&type!=32)return;
    if((u(p+24)&0x800)&&!(u(other+24)&0x800))return;
    if(!enough_hits(p,other))return;
    boom_effect(p,0x20);debris(p,3,true,true);sw(p+24,0);wb(0x8f89,b(0x8f89)&254);
    for(unsigned i=0;i<3;++i)sw(0x1eee + 2*i,0);
    distort_palette();
}
void wall(unsigned p,bool long_wall){
    unsigned row=table(p,0xee17);wb(long_wall?0x6386:0x6200,long_wall&&s(row)<0?b(0x31b6):u(row));
    if(u(0xf0e6)){if(!u(0xf0e2))sw(p+24,0);return;}
    if(long_wall)sw(p+52,32000);
    else if(!quality(p)&&u(p+52)<=31992){sw(p+52,31990);sw(p+44,0x59ea);}
}
void man(unsigned p){
    sw(0x6d13,u(p+24)&0x800?0:1);sw(0x6d15,u(p+24)&0x800?0:6);
    if(u(p+24)&0x1000){for(unsigned at=40;at<=44;at+=2)sw(p+at,0x7099);sw(p+14,16384);}
    sw(p+52,32000);relative(p,0xb05);
    if(relative_mask||!b(0xafc)||u(p+40)!=0x6ea9)return;
    for(unsigned i=0;i<3;++i)sw(0x71c9 + 2*i,u(0xe7e + 4*i));
    if(magnitude(0x71c9)>2000)return;
    for(unsigned at=40;at<=44;at+=2)sw(p+at,0x6f71);
    if(u(p+24)&0x800)cc_complete_objective(p);
}
void man_hit(unsigned p,unsigned other){
    if((u(p+24)&0x1000)||(u(0x2d44 + 2*u(other+72))&8))return;
    unsigned hp=b(p+27);if(!enough_hits(p,other))return;
    wb(p+27,hp);sw(p+24,(u(p+24)|0x1000)&0xfff7);
    if(other==65535||(!b(other+19)&&(b(other+72)==30||b(other+72)==21||(u(other+72)==20&&!(u(other+24)&2)))))inc(0x22e);
}
void hut(unsigned p){
    sw(p+52,32000);unsigned distance=0;
    if(b(0xafc)&&pilot_distance(p,distance)&&distance<=2000){wb(0xf50,0);wb(0x1ca,255);}
}
unsigned hop(unsigned value,unsigned n){int result=w(value+bound(n)-(n>>1));return uint16_t(result-sar(result,3));}
void alien(unsigned p){ // altim c4cc
    unsigned row=table(p,0xedc3);wb(0x2e31,u(row));inc(p+52);
    if(!(u(p+24)&4)){if(b(p+18))sw(p+24,0);return;}
    inc(0xec59);if(b(0xafc)&&!b(p+18)){sw(p+52,16000);sw(p+24,u(p+24)|8);}
    if((u(p+52)&16384)||!(u(p+24)&8)||l(p+8)>0)return;
    sw(p+50,u(row+4));sl(p+8,0);wb(p+64,u(row+2));sw(p+46,hop(u(p+46),u(row+6)));sw(p+48,hop(u(p+48),50));sw(p+12,hop(u(p+12),1000));
}
void bomber_site(unsigned p){
    inc(0xec53);unsigned row=table(p,0xef61);wb(0x68e2,u(row));wb(0x68e3,u(row+2));sw(0xec34,u(row+4));
    if(u(p+52)>=100)return;
    if(u(0xec55)<=u(0xec51)||random()>uint16_t(wa(100))){inc(p+52);return;}
    sw(0xec30,quality(p));sw(0x8fa2,p);sw(0xec28,u(0x2cca + 100));unsigned child=cc_new_effect(50);
    if(child){sw(0xec28,child);wb(child+18,0);sw(child+52,32000);sw(child+24,u(child+24)|0x8000|((u(0xec30)&7)<<12));zero_velocity(child);for(unsigned at=12;at<=16;at+=2)sw(child+at,0);copy(p,child,12);}
    inc(0xec51);sw(p+52,u(0xec34));
}
void incendiaries(unsigned p,unsigned count){ // strtincend a156
    for(unsigned i=0;i<count;++i){unsigned child=cc_new_effect(10);if(!child)return;copy(p,child,12);sw(child+8,u(child+8)+750);copy(p+46,child+46,6);
        sw(child+46,bound(300)-150);sw(child+48,bound(300)-150);sw(child+50,bound(1200));wb(child+18,0);sw(child+52,uwa(150));sw(child+24,u(child+24)|0x8000);for(unsigned j=0;j<3;++j)sw(child+54+2*j,bound(4000));}
}
void big_boom(){if(uint16_t(raw_ticks()-u(0xf161))>=512){sw(0xf161,raw_ticks());cc_audio_big_boom();}}
void incendiary_hit(unsigned p){boom_effect(p,0x20);debris(p,3,false);sw(p+24,0);}
void bomber_hit(unsigned p,unsigned other){
    wb(0xec4e,b(p+27));
    if(other!=65535&&!enough_hits(p,other)){int difference=int8_t(uint8_t(b(0xec4e)-b(p+27)));if(difference>=0)sw(p+16,u(p+16)+difference*400);return;}
    inc(0x1ea);boom_effect(p,0);debris(p,9,true);sw(p+24,u(p+24)&0xfff7);for(unsigned i=0;i<3;++i)sw(p+54+2*i,bound(4000));if(u(p+72)==53)cc_complete_objective(p);
}
void start_vector(unsigned p){
    for(unsigned i=0;i<2;++i)sl(0xe7e + 4*i,d(int64_t(l(0x2a9 + 4*i))-l(p+4*i)));
    unsigned shifts=0;
    while(shifts<31&&(absolute32(l(0xe7e))>10||absolute32(l(0xe82))>10)){sl(0xe7e,sar(l(0xe7e),1));sl(0xe82,sar(l(0xe82),1));++shifts;}
    wb(0x68e4,shifts<=14?0:12);sw(0xf110,shifts);sw(0xec4c,shifts);
}
void bomber_shells(unsigned p){
    constexpr unsigned temp=0xf4c3;copy(p,temp,12);for(unsigned i=0;i<3;++i)sw(temp+12+2*i,0);
    unsigned q=quality(p);if(q>5)q=5;sw(temp+24,(u(p+24)&0x8fff)|(q<<12));sw(temp+18,u(p+18));aa(temp,true);
}
void bomber(unsigned p){ // bmrtim bf02, shared type50/53
    inc(0xec4f);unsigned row=table(p,0xefe7);sw(0xf104,u(row+12));wb(0x68e0,u(row));wb(0x68e2,u(row));wb(0x68e1,u(row+2));wb(0x68e3,u(row+2));wb(0x1b8b,b(raw_ticks()&512?0x68e0:0x68e1));
    if(radar(p)&&(u(p+24)&8)&&u(0xf0cb)<=u(0xf0c9)){
        sw(0xf0c9,u(0xf0cb));unsigned frequency=(unsigned(b(p+27))*256)/51;if(frequency>255)invalid=true;wb(0xf0cd,frequency+2);
    }
    bool abort=false;
    if(u(p+72)==53){
        wb(0x68e2,b(0x31b6));wb(0x68e3,b(0x31b6));
        if(u(p+52)>=32700){if(!(u(p+24)&0x4000))sw(p+52,32000);else if(b(p+27)>=25){sw(p+52,32767);abort=true;}else{wb(p+27,50);sw(p+52,32000);inc(0xf0e0);}}
    }
    if(b(0xeaf)&&b(p+27)>=49)abort=true;
    if(!abort&&!(u(p+24)&8)){
        if(d(int64_t(l(p+8))-1000)<0){debris(p,12,false);incendiaries(p,15);sw(p+24,0);big_boom();}
        wb(p+64,15);sw(p+60,20);sw(p+62,18000);abort=true;
    }
    if(!abort){
        int life=w(u(p+52)+1-wa(3));sw(p+52,life<0?0:life);sw(p+16,u(p+16)-sar(s(p+16),4));sw(p+12,u(p+12)+4000);sw(0xec2e,u(p+14));
        int adjustment=divide(w(-w(uint32_t(d(int64_t(l(p+8))-3*65536))>>8)),100);if(absw(adjustment)>=40)adjustment=clamp(adjustment,-15,15);sw(p+50,s(p+50)+wa(adjustment));int damping=uwa(20);if(!damping)damping=1;sw(p+50,s(p+50)-divide(s(p+50),damping));
        start_vector(p);unsigned mode=u(p+24)&6;sw(0xec48,mode);
        switch(u(0xec65+mode)){
        case 0xbe86:wb(0x67b1,2);sw(p+46,u(p+46)+u(0xe7e)*2);sw(p+48,u(p+48)+u(0xe82)*2);sw(0xec4a,u(0xec4c)<=13?50:b(p+27)<=25?20:3);break;
        case 0xbed6:wb(0x67b1,0);sw(p+46,u(p+46)-u(0xe7e));sw(p+48,u(p+48)-u(0xe82));sw(0xec4a,u(row+14));break;
        case 0xbeb8:wb(0x67b1,1);sw(p+48,u(p+48)+u(0xe7e));sw(p+46,u(p+46)-u(0xe82));sw(0xec4a,u(row+18));break;
        case 0xbef2:wb(0x67b1,3);sw(0xec4a,u(row+20));break;
        }
        if((random()>>1)<=uint16_t(wa(s(0xec4a))))sw(p+24,(u(p+24)&0xfff9)|((mode+2)&6));
        if(u(0xec4c)<=13){
            if(u(p+10)&&(random()>>1)<=u(row+22)){
                unsigned child=cc_new_effect(20);if(child){wb(child+18,b(p+18));wb(child+19,2);sw(child+52,3200);sw(child+24,u(child+24)|0x8000);copy(p,child,12);copy(p+46,child+46,6);}sw(u(0xec28)+24,u(u(0xec28)+24)|2);
            }
        }else if((random()>>1)<=u(row+24)&&(u(p+24)&8)&&b(0x1dc)<=b(row+26)){
            sw(0xec30,quality(p));inc(0x1dc);sw(0x8fa2,p);unsigned child=cc_new_effect(24);
            if(child){wb(child+18,0);sw(child+52,24000);sw(child+24,u(child+24)|0x8000|((u(0xec30)&7)<<12));zero_velocity(child);sw(child+12,0);sw(child+14,-16384);sw(child+16,0);copy(p,child,12);}
            sw(0xec32,row);child=u(0xec28);wb(child+64,20);sw(child+60,20);sw(child+62,16000);sw(child+52,uwa(60)+23900);
        }
        unsigned elapsed=uint16_t(32000-u(p+52));if(elapsed>=1000){sw(p+52,32000);elapsed=0;}elapsed=uint16_t(wa(elapsed));
        if(u(row+6)){int change=hm(-sine(uint16_t(elapsed*u(row+4)*8)),s(row+6));if(u(p+10))sw(p+50,s(p+50)+change);}
        if(u(row+10)){unsigned arg=uint16_t(elapsed*u(row+8)*8);sw(p+46,s(p+46)+hm(-sine(arg),s(row+10)));sw(p+48,s(p+48)+hm(-cosine(arg),100));}
        unsigned oldspeed=magnitude(p+46);sw(0xf0ee,oldspeed);sw(0xf0ec,oldspeed);int correction=divide(w(u(0xf104)-oldspeed),s(row+8));sw(0xf0ec,u(0xf0ec)+correction+w(bound(u(row+16))-(u(row+16)>>1)));
        unsigned speed=u(0xf0ec);if(speed<=oldspeed&&oldspeed){if(w(speed)>w(oldspeed*2)){speed=2;oldspeed=1;}for(unsigned i=0;i<3;++i)sw(p+46+2*i,divide(s(p+46+2*i)*w(speed),w(oldspeed)));}
    }
    if((u(p+24)&8)&&b(u(0xec32)+27))bomber_shells(p);
}

void bomb_hit(unsigned p){
    boom_effect(p,0x60);incendiaries(p,u(p+24)&2?5:15);big_boom();
    if(u(p+10)||u(p+8)>10000){sw(p+24,0);return;}
    for(unsigned at=40;at<=44;at+=2)sw(p+at,0x72c6);
    sw(p+72,32);sl(p+8,10);wb(p+18,3);for(unsigned at=12;at<=16;at+=2)sw(p+at,0);wb(p+26,0);
    unsigned life=uint16_t(uwa(s(0xf0fe)));sw(0xf100,life-100);sw(p+52,u(p+24)&2?life>>4:life);sw(p+24,u(p+24)&0x911);
}
void bomb(unsigned p){
    wb(0x393b,u(p+24)&0x800?15:7);
    if(!(u(p+24)&4)){
        if(l(p+8)<=0){bomb_hit(p);return;}
        if(observer(p,false)&&u(0xf102)>=200)sw(p+24,u(p+24)|0xe8);
    }
    if(u(p+24)&4){if(!(b(0x2cb)&1))return;sw(p+24,(u(p+24)|0xe8)&0xfffb);sw(p+66,6000);}
    int speed=w(-int(magnitude(p+46)));for(unsigned i=0;i<3;++i)sw(p+46+2*i,s(p+46+2*i)+divide(s(p+46+2*i)*speed,4630)-(i==2?54:0));
}
void crater(unsigned p){
    wb(0x7291,0);if(u(p+24)&0x800){inc(p+52);sw(p+24,u(p+24)|0xe0);wb(0x7291,6);}
    else if(u(0xf100)>=u(p+52))sw(p+24,u(p+24)|0xe0);
}
void portal(unsigned p){
    unsigned row=table(p,0xee29);wb(0x2dd3,u(p+24)&0x200?15:u(row));unsigned alternate=u(row+2);
    if(w(random())>=0)wb(0x2dd3,alternate);
    unsigned life=u(p+52);sw(p+52,32000);if(!(life&16384))wb(0x2dd3,alternate);
}
void portal_hit(unsigned p,unsigned other){
    if(other==65535&&!b(0xafc)){
        unsigned row=table(p,0xee29),flags=u(row+4);wb(0x8f88,flags);if(flags&0x100)cc_complete_objective(p);
        int damage=s(row+6);
        if(damage<0)wb(0xe58,255);
        else if(damage&2){
            cc_aircraft_clear_damage();cc_weapon_restore_view();
        }else if(damage){if(cc_aircraft_damage(75))invalid=true;}
    }
    sw(p+52,u(p+52)&16383);
}
unsigned distance_object(unsigned p){
    relative(p,0xb05);if(relative_mask&0xff00){wb(p+18,3);return 1;}
    unsigned code=relative_mask&0xf0?2:relative_mask?1:0;wb(p+18,code);uint32_t square=0;
    for(unsigned i=0;i<3;++i){int value=w(uint32_t(l(0xe7e + 4*i))>>(code*4));square+=uint32_t(value*value);}sl(p+20,d(square));return 0;
}
void rescue(){
    copy(0x2a9,0x1a1e,8);sw(0xec30,7);sl(0x1a26,50);for(unsigned i=0;i<3;++i)sw(0x1a2a+2*i,0);
    sw(0x8fa2,0x1a1e);unsigned p=cc_new_effect(48);if(p){wb(p+18,0);sw(p+24,u(p+24)|0xf000);copy(0x1a1e,p,18);}
    p=u(0xec28);wb(p+18,3);
    for(unsigned i=0;i<16;++i){if(!(b(p+18)&0xfe))return;distance_object(p);for(unsigned j=0;j<2;++j)addl(p+4*j,-int64_t(sar(l(0xe7e + 4*j),1)));}
    source_error(75);
}
void change_wall_table(unsigned start,unsigned count,int32_t value){
    // The source tables contain at least one address; zero would LOOP 65536.
    if(!count){invalid=true;return;}
    for(unsigned i=0;i<count;++i){unsigned dest=u(start+2*i);sw(dest,s(dest)<0?-value:value);}
}
void wall_size(unsigned ptr,unsigned first,unsigned second,int32_t value){
    change_wall_table(ptr,u(first),value);change_wall_table(ptr+2*u(first),u(second),w(value*2));
}
}
extern "C" uint32_t cc_enemy_distance_object(uint32_t p){
    if(!valid_object(p))return 2;
    invalid=false;unsigned status=distance_object(p);return invalid?2:status;
}
extern "C" uint32_t cc_enemy_rescue_request(){
    if(b(0x26c)!=0xaf)return 0;
    wb(0x26c,0);
    if(!b(0xafc)||!b(0xf79)||u(0xec46)||b(0xf55))return 0;
    invalid=false;rescue();return invalid?2:0;
}
extern "C" uint32_t* cc_enemy_radar_event(){return radar_event;}
extern "C" uint32_t* cc_enemy_palette_event(){return palette_event;}
extern "C" uint32_t cc_enemy_observer_vector(uint32_t p,uint32_t force){
    if(!valid_object(p))return 2;
    invalid=false;bool near=observer(p,force!=0);return invalid?2:near?0:1;
}
extern "C" uint32_t cc_enemy_attract(uint32_t p){
    if(!valid_object(p)||!valid_time())return 2;
    invalid=false;attract(p);return invalid?2:0;
}
extern "C" uint32_t cc_enemy_shell_hit(uint32_t p){
    if(!valid_object(p)||!valid_time())return 2;
    if(u(p+72)!=49)return 1;
    invalid=false;
    unsigned row=table(p,0xefb5),type=u(row);copy(p,0x1a1e,18);unsigned prototype=u(0x2cca + type*2);sw(0x1a26,u(prototype+8));sw(0xec30,7);
    sw(0x8fa2,0x1a1e);unsigned child=cc_new_effect(type);if(child){wb(child+18,0);sw(child+24,u(child+24)|0xf000);copy(0x1a1e,child,18);}
    sw(p+24,0);shell(p);return invalid?2:0;
}
extern "C" uint32_t cc_enemy_timed(uint32_t p){
    if(!valid_object(p)||!valid_time())return 2;
    invalid=false;unsigned status=0;
    switch(u(p+72)){
    case 0:status=hq(p,false);break;
    case 1:distortion(p);break;
    case 2:status=parked_plane(p);break;
    case 3:sw(p+12,raw_ticks()<<2);inc(p+52);break;
    case 5:road(p);break;
    case 6:tower(p);inc(p+52);break;
    case 7:man(p);break;
    case 8:case 9:shard(p);break;
    case 10:if(!u(p+8))incendiary_hit(p);break;
    case 11:site(p,false);break;
    case 12:case 48:vehicle(p);break;
    case 13:site(p,true);break;
    case 14:case 15:case 16:case 17:wall(p,false);break;
    case 18:break; // pufftim ac0a is RET.
    case 19:
        if(s(p+52)<=3)for(unsigned at=40;at<=44;at+=2)sw(p+at,s(p+52)>2?0x3d64:0x3d3c);
        break;
    case 20:bomb(p);break;
    case 21:
        if(!l(p+8)){if(cc_combat_hit(p,0)==2)invalid=true;}
        else if(s(p+52)>=89&&magnitude(p+46)<=3000)for(unsigned i=0;i<3;++i)sw(p+46+2*i,divide(s(p+46+2*i)*1000,890));
        break;
    case 22:aa_shell(p);break;
    case 23:aa(p);break;
    case 24:kla(p);break;
    case 25:case 26:if(!l(p+8))sw(p+24,0);break;
    case 27:inc(p+52);inc(0xec3c);break;
    case 28:
        sw(p+52,32000);if(p!=0x5bb0){sw(p+72,2);status=parked_plane(p);break;}
        wb(0x44ce,u(p+24)&0x1000?6:1);if(u(p+24)&0x2000){sw(p+40,0x6838);sw(p+42,0x6838);}break;
    case 31:sw(p+52,32000);break;
    case 32:crater(p);break;
    case 34:hut(p);break;
    case 30:break; // bdest bbef does nothing when carry is clear.
    case 40:balloon(p,false);break;
    case 41:{bool carry;for(unsigned i=0;i<3;++i)sw(p+12+2*i,random_carry(carry));balloon(p,carry);break;}
    case 42:mortar(p);break;
    case 43:status=hq(p,true);break;
    case 49:shell(p);break;
    case 50:case 53:bomber(p);break;
    case 51:bomber_site(p);break;
    case 52:inc(p+52);inc(0xec5f);break;
    case 57:alien(p);break;
    case 58:portal(p);break;
    default:
        // Exact shared permanent-life callback and no-op timed callbacks only.
        if(u(p+72)<61&&u(0x2c50 + 2*u(p+72))==0xb805)wall(p,true);
        else if(u(p+72)<61&&u(0x2c50 + 2*u(p+72))==0x8e20)sw(p+52,32000);
        else return 1;
    }
    return invalid?2:status;
}
extern "C" uint32_t cc_enemy_hit(uint32_t p,uint32_t other){
    if(!valid_object(p)||(other!=65535&&!valid_object(other))||!valid_time())return 2;
    invalid=false;
    switch(u(p+72)){
    case 1:distortion_hit(p,other);break;
    case 2:
        if(other!=0x5bb0&&!(u(0x2d44 + 2*u(other+72))&16))friendly_hit(p,other);
        break;
    case 3:sw(p+12,raw_ticks()<<2);friendly_hit(p,other);break;
    case 5:road(p);break;
    case 6:tower(p);friendly_hit(p,other);break;
    case 7:man_hit(p,other);break;
    case 10:incendiary_hit(p);break;
    case 14:case 15:case 16:case 17:sw(p+44,u(p+40));sw(p+52,32000);break;
    case 18:case 32:case 34:break;
    case 19:if(s(p+52)<=3)for(unsigned at=40;at<=44;at+=2)sw(p+at,s(p+52)>2?0x3d64:0x3d3c);break;
    case 20:bomb_hit(p);break;
    case 11:
        if(!enough_hits(p,other)){unsigned row=table(p,0xed2f);wb(0x72ee,u(row));sw(0xec34,u(row+4));sw(p+52,u(0xec34));break;}
        boom_effect(p,0);inc(0x1b81);inc(0x21c);debris(p,9,true);boom_sound();sw(p+24,0);break;
    case 12:case 48:vehicle_hit(p,other);break;
    case 13:
        if(!enough_hits(p,other)){unsigned twice=uint16_t(u(0xf0f2)*2);sw(p+52,twice+(twice>>1));break;}
        boom_effect(p,0);inc(0x1b81);debris(p,9,true);boom_sound();table(p,0xee6b);inc(0x21c + 2*quality(p));sw(p+24,0);break;
    case 22:aa_shell(p,true);break;
    case 23:
        if(!enough_hits(p,other))break;
        boom_effect(p,0);inc(0x1b81);sw(0xf0de,w(u(0xf0de)-1)<0?0:u(0xf0de)-1);debris(p,4,false);boom_sound();table(p,0xed61);inc(0x1fc + 2*quality(p));sw(p+24,0);break;
    case 24:
        boom_effect(p,0);inc(0x1b81);debris(p,9,true);table(p,0xec6d);inc(0x1ec + 2*quality(p));sw(p+24,0);break;
    case 28:
        if(p==0x5bb0)cc_aircraft_destroy_abandoned();
        else {sw(p+24,1);sl(p+8,100);zero_velocity(p);debris(p,15,false);debris(p,15,false,true);sl(p+8,0);wb(0xf7b,255);sw(0x1de2,20);cc_world_funeral(1);big_boom();}break;
    case 31:sw(p+52,32000);break;
    case 40:balloon(p,true);break;
    case 41:{bool carry;for(unsigned i=0;i<3;++i)sw(p+12+2*i,random_carry(carry));balloon(p,carry);break;}
    case 42:
        if(!enough_hits(p,other))break;
        boom_effect(p,0);inc(0x1b81);inc(0x21c);debris(p,9,true);sw(p+24,0);break;
    case 43:{unsigned status=hq(p,true);return invalid?2:status;}
    case 49:return cc_enemy_shell_hit(p);
    case 50:case 53:bomber_hit(p,other);break;
    case 51:
        if(!enough_hits(p,other)){unsigned row=table(p,0xef61);wb(0x67b1,u(row));sw(0xec34,u(row+4));sw(p+52,u(0xec34));break;}
        boom_effect(p,0);inc(0x1b81);table(p,0xef61);inc(0x21c + 2*quality(p));debris(p,9,true);sw(p+24,0);break;
    case 58:portal_hit(p,other);break;
    case 57:
        if(u(other+72)==30&&enough_hits(p,other)){sl(p+8,0);sw(p+14,16384);sw(p+24,u(p+24)&0xfff3);wb(p+64,0);sw(p+50,100);inc(0x1ea);}break;
    case 52:
        if(enough_hits(p,other)){boom_effect(p,0x20);debris(p,3,true,true);inc(0x1ea);sw(p+24,0);}debris(p,2,true);break;
    default:
        if(u(p+72)<61&&u(0x2c50 + 2*u(p+72))==0xb805)break;
        else if(u(p+72)<61&&u(0x2c50 + 2*u(p+72))==0x8e20)sw(p+52,32000);
        else return 1;
    }
    return invalid?2:0;
}
extern "C" uint32_t cc_enemy_change_walls(){
    if(!valid_time())return 2;
    if((b(0x8f89)&2)&&!s(0x8f8a))return 2;
    invalid=false;inc(0x6312);sw(0x2dcc,u(0x6312)<<12);
    int32_t size=w(hm(cosine(u(0x2dcc)),8191)+8191);if(!size)size=1;if(size<0)size=16382;
    wall_size(0x6318,0x6314,0x6316,size);
    int32_t wave=cosine(uint16_t(u(0x6312)*3111));sw(0x2dca,wave);
    int32_t offset=hm(wave,333);sw(0x6a93,offset+999);sw(0x6a9f,999-offset);
    size=w(hm(wave,4095)+4095);if(!size)size=1;if(size<0)size=8191;wall_size(0x6342,0x633e,0x6340,size);
    size=w(hm(cosine(uint16_t(u(0x6312)*5333)),8191)+8191);if(!size)size=1;if(size<0)size=16383;wall_size(0x6378,0x6374,0x6376,size);
    if(b(0x8f89)&1){for(unsigned i=0;i<3;++i)sw(0x1eee + 2*i,u(0x1eee + 2*i)+hm(wa(s(0x276 + 2*i)),s(0x2dd1)));distort_palette();}
    int32_t x=300,y=200;sw(0xf0d3,128); // Original word store overlaps dballvector.x low byte.
    if(b(0x8f89)&2){
        int32_t amount=s(0x8f8a);x=w(300+hm(sine(u(0x2dca)),amount)-(u(0x8f8a)>>1));
        int32_t v=hm(sine(u(0x2dcc)),amount);sw(0xf0d3,divide(128*v,amount));y=w(200+v-(u(0x8f8a)>>1));
    }
    unsigned window=u(0x1c3e);sw(window+12,x);sw(window+14,y);return invalid?2:0;
}
extern "C" uint32_t cc_enemy_initnumbers(){
    if(!valid_time())return 2;
    if((b(0x8f89)&2)&&!s(0x8f8a))return 2;
    sw(0xf0ce,64);sw(0xf0d1,63);wb(0xf0d3,128);sw(0xf0c9,256);
    wb(0xf127,uint16_t(raw_ticks()-u(0xf125))>=2048?255:0);
    sw(0xf138,0);sw(0xf136,0);
    for(unsigned i=0;i<8;++i){sw(0xf136,u(0xf136)+u(0xf115 + 2*i)*(i+1));sw(0xf138,u(0xf138)+u(0xf13e + 2*i)*(i+1));}
    for(unsigned i=0;i<8;++i){
        if(u(0xf136)>u(0xf132))sw(0x242 + 2*i,u(0xf115 + 2*i));
        sw(0xf115 + 2*i,0);
        if(u(0xf138)>u(0xf134))sw(0x232 + 2*i,u(0xf13e + 2*i));
        sw(0xf13e + 2*i,0);
    }
    if(u(0xec59)>u(0xec5b))sw(0xec5b,u(0xec59));
    if(u(0xf138)>u(0xf134))sw(0xf134,u(0xf138));
    if(u(0xf136)>u(0xf132))sw(0xf132,u(0xf136));
    static const unsigned pairs[][2]={{0xec44,0xec46},{0xec3c,0xec3e},{0xec40,0xf113},{0xec42,0xf0ea},{0xf13c,0xf0e8},{0xf13a,0x1dc},{0xf0e0,0xf0e6},{0xec4f,0xec51},{0xec53,0xec55},{0xec59,0xec57},{0xec5d,0xec61},{0xec5f,0xec63},{0xf14e,0xf150},{0xf0e4,0xf0e2}};
    for(const auto& pair:pairs)sw(pair[1],u(pair[0]));
    for(const auto& pair:pairs)sw(pair[0],0);
    wb(0x8f94,0);return cc_enemy_change_walls();
}

extern "C" uint32_t cc_enemy_pilot_distance(uint32_t object){unsigned distance=0;return pilot_distance(uint16_t(object),distance)?distance:0x10000;}
