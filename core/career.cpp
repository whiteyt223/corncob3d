#include "career.hpp"
namespace {
constexpr uint32_t P=623;
uint16_t w(const uint8_t*p){return uint16_t(p[0]|uint16_t(p[1])<<8);}
uint32_t d(const uint8_t*p){return uint32_t(w(p))|(uint32_t(w(p+2))<<16);}
void sw(uint8_t*p,uint32_t v){p[0]=uint8_t(v);p[1]=uint8_t(v>>8);}
void sd(uint8_t*p,uint32_t v){sw(p,v);sw(p+2,v>>16);}
bool less(uint32_t a,uint32_t b){return (a^0x80000000u)<(b^0x80000000u);}
void zero(uint8_t*p,uint32_t n){for(uint32_t i=0;i<n;++i)p[i]=0;}
void copy(uint8_t*d,const uint8_t*s,uint32_t n){for(uint32_t i=0;i<n;++i)d[i]=s[i];}
struct Codec {uint32_t s;uint8_t*out;const uint8_t*in;uint32_t pos=4;
 uint8_t key(){s=s*0x015a4e35u+1;return uint8_t(s>>23);}
 void put(uint8_t v){out[pos++]=v^key();}
 void putw(uint32_t v){put(uint8_t(v));put(uint8_t(v>>8));}
 void putd(uint32_t v){putw(v);putw(v>>16);}
 uint8_t get(){return in[pos++]^key();}
 uint16_t getw(){uint16_t a=get();return uint16_t(a|(uint16_t(get())<<8));}
 uint32_t getd(){uint32_t a=getw();return a|(uint32_t(getw())<<16);}
};
uint8_t* at(uint8_t*p,uint8_t*t,int32_t i){return i<0?t:p+uint32_t(i)*P;}
void revive(uint8_t*p){sw(p+0x16,w(p+0x16)&~10u);sw(p+0x1a,w(p+0x1a)+1u);}
}
extern "C" {
uint32_t cc_career_file_size(uint32_t n){return n<=15?29+n*P:0;}
int32_t cc_career_encode(const uint8_t*m,const uint8_t*p,uint32_t n,uint8_t*out,uint32_t cap){
 uint32_t len=cc_career_file_size(n);if(!m||!out||(!p&&n)||!len)return -1;if(cap<len)return -2;
 sw(out,24);sw(out+2,w(m));Codec c{w(m),out,nullptr};uint32_t a=d(m+12),b=d(m+16);
 c.put(uint8_t(a>>24));c.put(uint8_t(b>>8));c.putw(w(m+2));c.put(uint8_t(a));c.put(uint8_t(b>>16));c.putw(w(m+4));c.put(uint8_t(a>>16));c.put(uint8_t(b));c.putd(d(m+20));c.putd(d(m+24));c.put(uint8_t(a>>8));c.put(uint8_t(b>>24));c.put(m[7]);c.put(m[8]);c.putw(n);
 if(n){for(uint32_t i=0;i<P;++i)c.put(p[i]);}
 c.put(m[6]);
 for(uint32_t i=P;i<n*P;++i)c.put(p[i]);
 return int32_t(len);
}
int32_t cc_career_decode_mode(uint32_t other_worlds,const uint8_t*file,uint32_t len,uint8_t*m,uint8_t*p,uint32_t cap){
 if(!file||!m||len<29||w(file)!=24)return -1;
 // Count is encoded before pilots. Validate before mutating caller outputs.
 Codec probe{w(file+2),nullptr,file};for(uint32_t i=0;i<22;++i)probe.get();uint32_t n=probe.getw();
 if(n>15||len!=cc_career_file_size(n)||(!p&&n))return -1;
 if(cap<n*P)return -2;
 zero(m,28);sw(m,w(file+2));Codec c{w(file+2),nullptr,file};uint32_t a=uint32_t(c.get())<<24,b=uint32_t(c.get())<<8;
 sw(m+2,c.getw());a|=c.get();b|=uint32_t(c.get())<<16;sw(m+4,c.getw());a|=uint32_t(c.get())<<16;b|=c.get();sd(m+20,c.getd());sd(m+24,c.getd());a|=uint32_t(c.get())<<8;b|=uint32_t(c.get())<<24;m[7]=c.get();m[8]=c.get();c.getw();
 if(n){for(uint32_t i=0;i<P;++i)p[i]=c.get();}
 m[6]=c.get();for(uint32_t i=P;i<n*P;++i)p[i]=c.get();sd(m+12,a);sd(m+16,b);if(other_worlds)sw(m+6,0x2add);return int32_t(n);
}
int32_t cc_career_active(const uint8_t*p,uint32_t n){if(n>15||(!p&&n))return -2;int32_t active=-1;for(uint32_t i=0;i<n;++i)if(w(p+i*P+0x16)&0x8000)active=int32_t(i);return active;}
int32_t cc_career_name_gate(const uint8_t*p,uint32_t n,const uint8_t*name,uint32_t len){
 if(n>15||(!p&&n)||!name||!len||len>20)return -1;
 if(n==15)return -4;
 for(uint32_t i=0;i<len;++i)if(!name[i])return -1;
 for(uint32_t j=0;j<n;++j){const uint8_t*q=p+j*P;uint32_t i=0;while(i<len&&q[i]==name[i])++i;if(i==len&&!q[i])return -3;}return 0;
}
int32_t cc_career_new(uint8_t*p,const uint8_t*name,uint32_t len,uint32_t trainee){
 if(!p||!name||!len||len>20)return -1;
 for(uint32_t i=0;i<len;++i)if(!name[i])return -1;
 zero(p,P);copy(p,name,len);const uint8_t opts[8]={5,6,2,0,0,4,0,2};copy(p+0x234,opts,8);if(trainee)p[0x23b]=10;p[0x23d]=0x80;return 0;
}
int32_t cc_career_select_gate(const uint8_t*p,uint32_t now){if(!p)return -1;uint16_t f=w(p+0x16);if(f&10)return 2;return (f&1)||less(now,d(p+0x26b))?1:0;}
int32_t cc_career_activate(uint8_t*p,uint32_t n,uint8_t*t,int32_t old,int32_t selected){
 if(n>15||(!p&&n)||!t||old< -1||selected< -1||old>=int32_t(n)||selected>=int32_t(n))return -1;
 uint8_t*a=at(p,t,old),*b=at(p,t,selected);sw(a+0x16,w(a+0x16)&0x7fff);if(selected>=0)sw(b+0x16,w(b+0x16)|0x8000);return 0;
}
int32_t cc_career_stockade(uint8_t*p,uint32_t now){
 if(!p)return -1;
 uint32_t k=w(p+0x66);if((w(p+0x16)&10)||!k)return 0;if(k&0x8000)k|=0xffff0000;
 uint32_t until=d(p+0x26b);if(less(until,now))until=now;sd(p+0x26b,until+k*86400u);return 1;
}
int32_t cc_career_resurrect(uint8_t*p,uint32_t now,uint32_t menu){if(!p)return -1;if(menu&&less(now,d(p+0x26b)))return 1;if(!(w(p+0x16)&10))return 2;revive(p);return 0;}
int32_t cc_career_resurrect_theater(uint8_t*p){if(!p)return -1;if(!(w(p+0x16)&10))return 0;revive(p);return 1;}
int32_t cc_career_after_sortie(uint8_t*p,uint8_t*t,uint32_t is_trainee,uint32_t now){
 if(!p||!t)return -1;
 int32_t switch_active=0;
 if(is_trainee){sw(p+0x16,w(p+0x16)&~10u);sd(p+0x26b,0);}
 else if((w(p+0x16)&10)||less(now,d(p+0x26b))){sw(p+0x16,w(p+0x16)&0x7fff);sw(t+0x16,w(t+0x16)|0x8000);switch_active=1;}
 if((w(p+0x16)&10)&&less(now,d(p+0x26b)))sd(p+0x26b,now);
 return switch_active;
}
int32_t cc_career_new_theater(const uint8_t*p,const uint8_t*def,uint8_t*h,uint8_t*t){
 if(!p||!def||!h||!t)return -1;
 uint32_t len=0;while(len<22&&p[len])++len;if(len==22)return -1;
 zero(t,P);copy(t,p,len);t[0x256]=p[0x256];copy(h,def,48);return 0;
}
int32_t cc_career_link_theater(uint8_t*p,uint32_t id){if(!p||!id||id>=30000||p[0x23c]>=20)return -1;uint8_t n=p[0x23c];p[0x23e + n]=uint8_t(id);p[0x23d]=n;p[0x23c]=uint8_t(n+1);return 0;}
int32_t cc_career_select_theater(uint8_t*p,uint32_t i){if(!p||i>=p[0x23c])return -1;p[0x23d]=uint8_t(i);return 0;}
}

extern "C" int32_t cc_career_decode(const uint8_t* f,uint32_t n,uint8_t* m,uint8_t* p,uint32_t cap){return cc_career_decode_mode(0,f,n,m,p,cap);}
