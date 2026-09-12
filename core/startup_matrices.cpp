#include "startup.hpp"
extern "C" {int32_t cc_ssin(uint32_t);int32_t cc_scos(uint32_t);uint32_t cc_cos(uint32_t);void cc_calcmat(const uint16_t*,uint16_t*);void cc_ncalcmat(const uint16_t*,uint16_t*);}
namespace {
uint16_t w(const uint8_t*p,uint32_t a){return uint16_t(p[a]|uint16_t(p[a+1])<<8);}
void sw(uint8_t*p,uint32_t a,uint32_t v){p[a]=uint8_t(v);p[a+1]=uint8_t(v>>8);}
void trig(uint8_t*ds,uint32_t angles,uint16_t*t){for(uint32_t i=0;i<3;++i){t[2*i]=uint16_t(cc_ssin(w(ds,angles+2*i)));t[2*i+1]=uint16_t(cc_scos(w(ds,angles+2*i)));}}
void write(uint8_t*ds,uint32_t p,const uint16_t*m){for(uint32_t i=0;i<9;++i)sw(ds,p+i*2,m[i]);}
}
extern "C" void cc_startup_matrices(uint8_t*ds){
 uint16_t t[6],m[9];trig(ds,0x1d7e,t);cc_calcmat(t,m);write(ds,0x1d12,m);
 trig(ds,0xb11,t);cc_calcmat(t,m);write(ds,0x1fde,m);write(ds,0x1d36,m);
 cc_ncalcmat(t,m);write(ds,0x1ff0,m);write(ds,0x1d48,m);write(ds,0x1d24,m);
 for(uint32_t i=0;i<6;++i)sw(ds,0x20ae + i*2,i%2?t[i]:0u-t[i]);
 ds[0xf378]=uint8_t(cc_cos(w(ds,0xb15))>>16);
}
extern "C" void cc_startup_recalcmats(uint8_t*ds){
 uint16_t t[6],m[9];trig(ds,0xb11,t);cc_calcmat(t,m);write(ds,0x1fde,m);
 cc_ncalcmat(t,m);write(ds,0x1ff0,m);
 for(uint32_t i=0;i<6;++i)sw(ds,0x20ae + i*2,i%2?t[i]:0u-t[i]);
 ds[0xf378]=uint8_t(cc_cos(w(ds,0xb15))>>16);
}
