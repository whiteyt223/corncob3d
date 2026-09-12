#include "mission_flow.hpp"
namespace {
uint16_t rd16(const uint8_t* p) { return uint16_t(p[0] | uint16_t(p[1])<<8); }
void cp(uint8_t* d,const uint8_t* s,uint32_t n) { for(uint32_t i=0;i<n;++i)d[i]=s[i]; }
}
extern "C" {
uint32_t cc_flow_shortcut_gate(uint32_t pending,uint32_t damage) {
 return !(pending&255) ? 0 : (damage&65535)<16 ? 1 : 2;
}
uint32_t cc_flow_escape(uint32_t radio,uint32_t close,uint32_t warned,uint32_t ejected) {
 if(!(radio&255)||(close&255)||(warned&255))return 0;
 return (ejected&255)?2:1;
}
uint32_t cc_flow_finalize_status(uint32_t status,uint32_t f) {
 status &=65535;
 if(f&1)return status|0x4000;
 if((f&2)&&!(f&4))status|=0x2000;
 if(f&8)return status;
 status&=~1u;
 if(f&16)status|=(f&32)?1:16;
 if(f&64)status=(status|256)&~8u;
 return status;
}
uint32_t cc_flow_home(uint32_t old,uint32_t close,uint32_t never,uint32_t frame) {
 uint32_t now=((old&255)|((close&255)>>1))&(close&1);
 if(!now&&(frame&65535)>=20)never=0;
 return now|((never&255)<<8);
}
uint32_t cc_flow_commit_gate(uint32_t status,uint32_t option,uint32_t overrideWord,uint32_t error,uint32_t choice) {
 if(((option&1)||(status&0x2000))&&!(overrideWord&65535))return 1;
 if(error&65535)return choice==1?0:choice==2?3:2;
 return status&0x4000?4:0;
}
uint32_t cc_flow_career_flags(uint32_t flags,uint32_t status) {
 return (flags | ((status&2)?2:(status&8)?8:0))&65535;
}
void cc_flow_theater_totals(uint16_t *v,uint32_t plane,uint32_t nhq) {
 if(!v)return;
 if(plane&65535)v[0]=uint16_t(v[0]-1);
 v[2]=uint16_t(v[2]+nhq);
 uint16_t left=uint16_t(v[1]-nhq);
 v[1]=(left&32768)?0:left;
}
int32_t cc_flow_start_marker(const uint8_t *line,uint32_t length,uint8_t *out,uint32_t cap) {
 if(!line||!out)return -1;
 static const uint8_t marker[6]={'S','x','y','t','h',' '};
 if(length<6)return 0;
 for(uint32_t i=0;i<6;++i)if(line[i]!=marker[i])return 0;
 uint32_t n=0;while(n+6<length&&line[n+6])++n;
 if(cap<=n)return -1;
 cp(out,line+6,n);out[n]=0;return int32_t(n+1);
}
uint32_t cc_flow_cfg_active(const uint8_t *cfg,uint32_t size,uint32_t *out) {
 if(!cfg||!out||size<679)return 1;
 uint32_t n=rd16(cfg+675), i=rd16(cfg+677);
 if(n>=32768||(i!=65535&&i>=n))return 1;
 uint32_t trainee=679+623*n;
 if(size<trainee+623)return 1;
 out[0]=n;out[1]=i==65535?0xffffffffu:i;
 out[2]=i==65535?trainee:679+623*i;out[3]=trainee;return 0;
}
uint32_t cc_flow_encode_theater(const uint8_t *h,const uint8_t *p,const uint8_t *w,uint32_t n,uint8_t *out,uint32_t cap) {
 if(!h||!p||(!w&&n)||!out||n>0xffffffffu-671||cap<671+n)return 0;
 cp(out,h,48);cp(out+48,p,623);if(n)cp(out+671,w,n);return n+671;
}
}
