#include "career_scores.hpp"
namespace {
uint16_t w(const uint8_t* p){return uint16_t(p[0]|uint16_t(p[1])<<8);}
int16_t iw(const uint8_t* p){return int16_t(w(p));}
int32_t l(const uint8_t* p){return int32_t(uint32_t(w(p))|uint32_t(w(p+2))<<16);}
void sw(uint8_t* p,uint16_t v){p[0]=uint8_t(v);p[1]=uint8_t(v>>8);}
void cp(uint8_t* d,const uint8_t* s,uint32_t n){for(uint32_t i=0;i<n;i++)d[i]=s[i];}
void strcp(uint8_t* d,const uint8_t* s,uint32_t n){cp(d,s,n);d[n]=0;}
bool eq(const uint8_t* a,const uint8_t* b,uint32_t n){for(uint32_t i=0;i<n;i++)if(a[i]!=b[i])return false;return a[n]==0;}
int cmp(int32_t a,int32_t b){return a>b?1:a<b?-1:0;}
int16_t ground(const uint8_t* a){uint16_t v=uint16_t(w(a+6)+w(a+0x48));for(uint32_t i=0;i<8;i++)v=uint16_t(v+w(a+0x38+2*i)+w(a+0x18+2*i)+w(a+0x72+2*i));return int16_t(v);}
int16_t air(const uint8_t* a){uint16_t v=0;for(uint32_t i=0;i<8;i++)v=uint16_t(v+w(a+0x28+2*i)+w(a+8+2*i));return int16_t(v);}
int compare(const uint8_t* a,const uint8_t* b){
 int c=cmp(l(a+0x82),l(b+0x82));if(c)return c;
 c=cmp(iw(a+0x48),iw(b+0x48));if(c)return c;
 c=cmp(ground(a),ground(b));if(c)return c;
 return cmp(air(a),air(b));
}
int fast(const uint8_t* p,const uint8_t* row){
 const int32_t pn=iw(p+0x18),rn=iw(row+0x16);
 if(!pn)return rn?-1:0;
 if(!rn)return 1;
 const int32_t pa=l(p+0x230),ra=l(row+0x9c);
 const int32_t q1=int32_t(int64_t(pa)/pn),q2=int32_t(int64_t(ra)/rn);
 const int32_t delta=int32_t(uint32_t(q1)-uint32_t(q2));
 return delta?cmp(delta,0):cmp(pa,ra);
}
int slot(uint8_t* list,uint32_t stride,const uint8_t* p,uint32_t name_n,int mode){
 if(mode)for(int i=0;i<10;i++)if(list[i*stride]&&eq(list+i*stride,p,name_n)&&w(list+i*stride+stride-2)==0)return i;
 for(int i=0;i<10;i++)if(!list[i*stride])return i;
 const uint8_t* last=list+9*stride;
 const int c=mode==2?fast(p,last):compare(p+(mode?0x1ae:0x1c),last+(mode?26:44));
 return c>0?9:-1;
}
}
extern "C" int32_t cc_career_scores_new(uint8_t* s,uint32_t n){if(!s||n!=5040)return -1;for(uint32_t i=0;i<n;i++)s[i]=0;return 0;}
extern "C" int32_t cc_career_scores_clear(uint8_t* s,uint32_t n,uint32_t list){if(!s||n!=5040||list>2)return -1;const uint32_t base=list==0?0:list==1?1800:3420,stride=list?162:180;for(uint32_t i=0;i<10;i++)s[base+i*stride]=0;return 0;}
extern "C" int32_t cc_career_scores_compare(const uint8_t* a,const uint8_t* b){return a&&b?compare(a,b):0;}
extern "C" int32_t cc_career_scores_fast_compare(const uint8_t* p,const uint8_t* row){return p&&row?fast(p,row):0;}
extern "C" int32_t cc_career_scores_update(uint8_t* s,uint32_t size,const uint8_t* p,uint32_t trainee,const uint8_t* theater,uint32_t tn){
 if(!s||size!=5040||!p)return -1;
 uint32_t pn=0;while(pn<21&&p[pn])pn++;if(!pn||pn>20)return -1;
 const uint8_t training[]="Training Mission";
 if(p[0x23d]&0x80){theater=training;tn=16;}
 else {if(!theater||tn>27)return -1;for(uint32_t i=0;i<tn;i++)if(!theater[i])return -1;}
 int mask=0;
 for(int mode=0;mode<3;mode++){
  if(mode&&trainee)break;
  if(mode==2&&iw(p+0x18)<3)break;
  const uint32_t base=mode==0?0:mode==1?1800:3420,stride=mode?162:180;
  uint8_t* list=s+base;int pos=slot(list,stride,p,pn,mode);if(pos<0)continue;
  int i=pos-1;
  while(i>=0){
   const int c=mode==2?fast(p,list+i*stride):compare(p+(mode?0x1ae:0x1c),list+i*stride+(mode?26:44));
   if(c<=0)break;
   cp(list+(i+1)*stride,list+i*stride,stride);--i;
  }
  uint8_t* row=list+(i+1)*stride;
  cp(row+(mode?26:44),p+(mode?0x1ae:0x1c),134);
  if(mode){sw(row+22,w(p+0x18));sw(row+24,w(p+0x1a));}
  strcp(row,p,pn);
  if(!mode)strcp(row+22,theater,tn);
  sw(row+stride-2,0);mask|=1<<mode;
 }
 return mask;
}
