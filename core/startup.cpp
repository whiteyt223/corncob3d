#include "startup.hpp"
namespace {
uint16_t w(const uint8_t* p,uint32_t a){return uint16_t(p[a]|uint16_t(p[a+1])<<8);}
void sw(uint8_t*p,uint32_t a,uint32_t v){p[a]=uint8_t(v);p[a+1]=uint8_t(v>>8);}
uint32_t d(const uint8_t*p,uint32_t a){return uint32_t(w(p,a))|uint32_t(w(p,a+2))<<16;}
void sd(uint8_t*p,uint32_t a,uint32_t v){sw(p,a,v);sw(p,a+2,v>>16);}
int32_t signedword(uint32_t a){a&=65535;return a<32768?int32_t(a):int32_t(a)-65536;}
void copy(uint8_t*to,const uint8_t*from,uint32_t n){for(uint32_t i=0;i<n;++i)to[i]=from[i];}
uint8_t ground[4566];
uint32_t randomword(uint8_t*ds){uint32_t v=d(ds,0xf722)*663608941u;sd(ds,0xf722,v);return v>>16;}
}
extern "C" void cc_startup_seed(uint8_t* ds,const uint8_t* image,uint32_t selector){copy(ds,image,65536);sw(ds,0x2dc0,selector);}
extern "C" void cc_startup_random_seed(uint8_t* ds,uint32_t ticks){sd(ds,0xf722,ticks|1);}
extern "C" uint32_t cc_startup_common_flags(const uint8_t* o,uint32_t g,uint32_t ace,uint8_t*out){
 uint32_t n=1;out[0]='-';
 if(g&1)out[n++]='g';
 if(g&2)out[n++]='i';
 if(o[7]&1)out[n++]='i';
 if(!(o[7]&32))out[n++]='y';
 if(!(o[7]&64))out[n++]='{';
 if(g&4)out[n++]='j';
 if(g&8)out[n++]='k';
 if(ace)out[n++]='f';
 if(n==1)n=0;
 out[n]=0;return n;
}
extern "C" void cc_startup_pilot_options(uint8_t*ds,const uint8_t*o,uint32_t g,uint32_t ace,uint32_t selected,uint32_t chain){
 uint8_t flags[16];uint32_t n=cc_startup_common_flags(o,g,ace,flags);
 for(uint32_t i=1;i<n;++i)switch(flags[i]){
 case 'g':ds[0xaf5]|=1;break;case 'i':ds[0xae6]=255;break;
 case 'y':ds[0xaed]=255;break;case '{':ds[0xaee]=255;break;
 case 'j':ds[0xaf5]|=128;break;case 'k':ds[0xaf6]=0;break;
 case 'f':ds[0xaec]=255;break;default:break;}
 ds[0xaf0]=255;
 if(selected)ds[0xaf1]=255;
 else{
  if(o[6])ds[0xae8]=255;
  if(o[3])sw(ds,0xe72,uint32_t(o[3])-1);
  if(o[5])sw(ds,0xe74,uint32_t(o[5])-1);
  if(o[4])sw(ds,0xe76,uint32_t(o[4])-1);
  sw(ds,0xe6c,o[0]);sw(ds,0xe6e,o[2]);sw(ds,0xe70,o[1]);
 }
 if(chain)ds[0xaea]=255;
}
extern "C" void cc_startup_load_cfg(uint8_t* ds,const uint8_t* cfg){
 copy(ds+0xaf7,cfg,52);uint16_t previous=w(ds,0xb25);
 for(uint32_t a=0xaf7;a<0xb25;a+=2){uint16_t encoded=w(ds,a);sw(ds,a,uint32_t(encoded)-previous);previous=encoded;}
}
extern "C" void cc_startup_before_world(uint8_t* ds){
 ds[0xae4]=ds[0xaf6];
 if(!(w(ds,0x1b8e)&0x8000)){sw(ds,0xb1f,w(ds,0x1b8c));sw(ds,0xb21,w(ds,0x1b8e));}
 ds[0xafc]=0;copy(ds+0xb05,ds+0x2a9,18);ds[0xf53]=255;ds[0xf52]=255;copy(ds+0xe8b,ds+0xb05,18);
 sw(ds,0x1d0,w(ds,0x5bee));sw(ds,0x1ce,w(ds,0x5bec));sw(ds,0x1d2,w(ds,0x5bf0));
}
extern "C" int32_t cc_startup_after_world(uint8_t* ds){
 if(!ds[0xaf1]||ds[0xae8]){ds[0xf4c]=0;return -1;}
 copy(ds+0xb05,ds+0x2a9,12);
 if(ds[0xaea]){ds[0xf4c]=0;sw(ds,0x1de,w(ds,0x1de)|0x1000);}
 else{
  sd(ds,0xb09,d(ds,0xb09)-3000);ds[0xafc]=255;ds[0xf79]=255;sw(ds,0x5bc8,0);
  if(w(ds,0x1c3e)!=0x1c26){sw(ds,0x1c40,0);ds[0xe57]=0;sw(ds,0x1c3e,0x1c26);}
  ds[0xae4]=0;
 }
 return 0;
}
extern "C" void cc_startup_generated_observer(uint8_t* ds){sd(ds,0xb09,d(ds,0xb09)-8000);sw(ds,0xb11,0);}
extern "C" void cc_startup_theater_params_mode(uint8_t*ds,const int32_t*v,uint32_t valid_last,uint32_t registered){
 sw(ds,0x280,uint32_t(v[0]));sw(ds,0x5386,uint32_t(v[1]));sw(ds,0x5390,uint32_t(v[1])+5);ds[0x537d]=uint8_t(v[2]&15);
 int32_t gravity=signedword(uint32_t(v[3]));if(gravity>=-15)gravity=-15;if(gravity<=-100)gravity=-100;sw(ds,0x1ddc,uint32_t(gravity));
 int32_t rho=signedword(uint32_t(v[4]));if(rho>=1000&&rho<=10000){sw(ds,0x1e1c,uint32_t(rho));sw(ds,0x1e22,uint32_t(rho*2)/3);}
 ds[0x1bad]=uint8_t(v[5]&1);int32_t rhoz=signedword(uint32_t(v[6]));
 if(valid_last&&rhoz>=256){sw(ds,0x1e1e,uint32_t(rhoz));sw(ds,0x27c,registered?uint32_t(rhoz)/2:uint32_t(rhoz*30)/50);}
}
extern "C" void cc_startup_sky_detail(uint8_t*ds){
 if(uint32_t(ds[0x1e6b])+ds[0x1e6c]+ds[0x1e6d]<=48){ds[0x1bae]=14;ds[0x537d]=15;}
}
extern "C" void cc_startup_palette(uint8_t*ds,const uint8_t*bios){
 copy(ds+0x1e6e,bios,64);uint64_t packed=0;
 for(uint32_t i=0;i<5;++i)packed=(packed<<8)|ds[0x1fae + i];
 if(packed){packed>>=2;const uint32_t offsets[6]={0x1e6a,0x1e69,0x1e68,0x1e6d,0x1e6c,0x1e6b};for(uint32_t off:offsets){ds[off]=uint8_t(packed&63);packed>>=6;}}
 for(uint32_t i=0;i<3;++i){ds[0x1e6e + 9+i]=ds[0x1e68 + i];ds[0x1e6e + 45+i]=ds[0x1e6b + i];}
}
extern "C" uint8_t* cc_startup_ground_buffer(){return ground;}
extern "C" void cc_startup_ground(uint8_t*ds,const uint8_t*source){
 copy(ground,source,4566);uint32_t count=w(ground,0);
 // Original data count80; reject malformed caller assets by preserving them.
 if(count!=80)return;
 for(uint32_t i=1;i<=count;++i){uint32_t p=2+i*14,z=d(ds,0xb0d);if(z&0x80000000u)continue;
  uint32_t extent=z<<6,half=(extent>>1)|(extent&0x80000000u);
  for(uint32_t a=0;a<2;++a)sd(ground,p+a*4,d(ds,0xb05 + a*4)+uint32_t((uint64_t(extent)*randomword(ds))>>16)-half);
  sd(ground,p+8,0);sw(ground,p+12,0);
 }
 for(uint32_t i=1;i<=count;++i){uint32_t p=0x47e + i*14;
  for(uint32_t a=0;a<2;++a){uint32_t high=randomword(ds)%6-3;uint32_t low=randomword(ds);sd(ground,p+a*4,(high<<16)|low);}
  sd(ground,p+8,0);sw(ground,p+12,1);
 }
 for(uint32_t i=1;i<=count;++i){uint32_t p=0x8fa + i*14;
  for(uint32_t a=0;a<2;++a){uint32_t high=randomword(ds)%16384-8192;uint32_t low=randomword(ds);sd(ground,p+a*4,(high<<16)|low);}
  sd(ground,p+8,0x08000000);sw(ground,p+12,1);
 }
}

extern "C" void cc_startup_theater_params(uint8_t*ds,const int32_t*v,uint32_t valid_last){cc_startup_theater_params_mode(ds,v,valid_last,0);}
