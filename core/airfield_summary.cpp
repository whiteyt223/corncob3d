#include "airfield_summary.hpp"
#include "edition.hpp"
namespace {
uint8_t rows[126];uint32_t result[4];
unsigned w(const uint8_t*p){return p[0]|unsigned(p[1])<<8;}
void sw(uint8_t*p,unsigned v){p[0]=uint8_t(v);p[1]=uint8_t(v>>8);}
void sl(uint8_t*p,uint32_t v){sw(p,v);sw(p+2,v>>16);}
void inc(uint8_t*p){sw(p,w(p)+1);}
uint32_t abs32(uint32_t n){return int32_t(n)<0?0u-n:n;}
const uint8_t regions[64]={0,0,0,3,3,6,6,6,0,0,0,3,3,6,6,6,0,0,0,3,3,6,6,6,1,1,1,4,4,7,7,7,1,1,1,4,4,7,7,7,2,2,2,5,5,8,8,8,2,2,2,5,5,8,8,8,2,2,2,5,5,8,8,8};
}
extern "C" uint8_t* cc_airfield_summary_buffer(){return rows;}
extern "C" uint32_t* cc_airfield_summary_result(){return result;}
static uint32_t analyze(const uint8_t*f,uint32_t size,uint32_t at,uint32_t plain,bool spies){
 for(auto&v:result)v=0;
 for(unsigned i=0;i<9;i++){auto*r=rows+i*14;sl(r,uint32_t(-5043272)+4194304u*(i%3));sl(r+4,uint32_t(-4194304)+4194304u*(i/3));sw(r+8,0);sw(r+10,0);sw(r+12,0);}
 unsigned towers=0;
 for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++){
  if(at>size||size-at<16){result[3]=1;return 1;}
  auto*h=f+at;unsigned count=w(h+8);if(w(h+4)!=x||w(h+6)!=y){result[3]=1;return 1;}at+=16;
  if(count>(size-at)/23){result[3]=1;return 1;}
  for(unsigned i=0;i<count;i++,at+=23){auto*p=f+at;unsigned type=w(p+21)^(plain?0:(w(p+2)^w(p+4))),flags=w(p+11);
   if(type==6)towers=(towers+1)&65535;
   bool objective=type==0||(type==58&&((flags&0x7000)==0x4000||(flags&0x7000)==0x7000))||type==53||type==43||(spies&&type==7&&(flags&0x800));
   bool remaining=objective&&!(flags&0x200),plane=type==2&&!(flags&0x100);
   if(objective){unsigned field=(flags&0x200)?1:0;result[field]=(result[field]+1)&65535;}
   if(plane){unsigned packed=w(p);uint32_t px=uint32_t(int32_t(int16_t(x*32-128+(packed&31))))*65536u+w(p+2),py=uint32_t(int32_t(int16_t(y*32-128+((packed>>5)&31))))*65536u+w(p+4),pz=((packed>>10)&31)*65536u+w(p+6);
    for(unsigned j=0;j<9;j++){uint32_t cx=uint32_t(-5043272)+4194304u*(j%3),cy=uint32_t(-4194304)+4194304u*(j/3);uint32_t mask=((abs32(px-cx)<<1)|(abs32(py-cy)<<1)|(abs32(pz-720)<<1))>>16;if(!(mask&0xfff0))inc(rows+j*14+8);}
   }else if(remaining)inc(rows+regions[x*8+y]*14+10);
  }
 }
 if(size-at<12){result[3]=1;return 1;}
 if(towers==1){result[2]=1;for(unsigned i=0;i<9;i++)if(i!=4){sw(rows+i*14+8,0);sw(rows+4*14+10,w(rows+4*14+10)+w(rows+i*14+10));sw(rows+i*14+10,0);}}
 return 0;
}

extern "C" uint32_t cc_airfield_analyze_file(const uint8_t*f,uint32_t size,uint32_t at){return cc_airfield_analyze_file_mode(f,size,at,cc_edition_is_other_worlds());}

extern "C" uint32_t cc_airfield_analyze_file_mode(const uint8_t*f,uint32_t size,uint32_t at,uint32_t plain){return analyze(f,size,at,plain,true);}
extern "C" uint32_t cc_airfield_analyze_file_tu(const uint8_t*f,uint32_t size,uint32_t at){return analyze(f,size,at,cc_edition_is_other_worlds(),false);}
