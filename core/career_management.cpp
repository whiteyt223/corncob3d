#include "career_management.hpp"
namespace {
constexpr uint32_t P=623,S=5040;
uint16_t w(const uint8_t* p){return uint16_t(p[0]|uint16_t(p[1])<<8);}
void sw(uint8_t* p,uint16_t v){p[0]=uint8_t(v);p[1]=uint8_t(v>>8);}
bool valid_name(const uint8_t* p,uint32_t n){if(!p||!n||n>20)return false;for(uint32_t i=0;i<n;i++)if(!p[i])return false;return true;}
uint32_t name_length(const uint8_t* p){uint32_t n=0;while(n<21&&p[n])++n;return n;}
bool same(const uint8_t* p,const uint8_t* s,uint32_t n){for(uint32_t i=0;i<n;i++)if(p[i]!=s[i])return false;return p[n]==0;}
void name_copy(uint8_t* p,const uint8_t* s,uint32_t n){for(uint32_t i=0;i<n;i++)p[i]=s[i];p[n]=0;}
int scores_change(uint8_t* scores,const uint8_t* old,uint32_t old_n,const uint8_t* name,uint32_t n){
 int changed=0;uint32_t base=0;
 for(uint32_t list=0;list<3;list++){
  const uint32_t stride=list?162:180;
  for(uint32_t i=0;i<10;i++){
   uint8_t* row=scores+base+i*stride;
   if(row[0]&&same(row,old,old_n)&&(!name||w(row+stride-2)==0)){
    if(name)name_copy(row,name,n);else sw(row+stride-2,1);
    ++changed;
   }
  }
  base+=stride*10;
 }
 return changed;
}
}
extern "C" uint32_t cc_career_scores_size(){return S;}
extern "C" int32_t cc_career_scores_rename(uint8_t* s,uint32_t size,const uint8_t* old,uint32_t old_n,const uint8_t* name,uint32_t n){
 if(!s||size!=S||!valid_name(old,old_n)||!valid_name(name,n))return -1;
 return scores_change(s,old,old_n,name,n);
}
extern "C" int32_t cc_career_scores_delete(uint8_t* s,uint32_t size,const uint8_t* name,uint32_t n){
 if(!s||size!=S||!valid_name(name,n))return -1;
 return scores_change(s,name,n,nullptr,0);
}
extern "C" int32_t cc_career_rename_pilot(uint8_t* p,uint32_t count,uint32_t selected,const uint8_t* name,uint32_t n,uint8_t* s){
 if(!p||count>15||selected>=count||!valid_name(name,n))return -1;
 for(uint32_t i=0;i<count;i++)if(same(p+i*P,name,n))return -3;
 uint8_t* row=p+selected*P;const uint32_t old_n=name_length(row);
 if(old_n>20)return -1;
 if(s&&old_n)scores_change(s,row,old_n,name,n);
 name_copy(row,name,n);return 0;
}
extern "C" int32_t cc_career_delete_pilot(uint8_t* p,uint8_t* state,uint32_t selected,uint8_t* s,uint8_t* ids){
 if(!p||!state||!ids)return -1;
 const uint32_t count=w(state);int active=int16_t(w(state+2));
 if(count>15||selected>=count||active < -1||active>15)return -1;
 uint8_t* row=p+selected*P;const uint32_t n=row[0x23c],old_n=name_length(row);
 if(n>20||old_n>20)return -1;
 for(uint32_t i=0;i<20;i++)ids[i]=i<n?row[0x23e + i]:0;
 if(active==int(selected))active=-1;
 if(s){
  if(old_n)scores_change(s,row,old_n,nullptr,0);
  // c3c5 loads the fastest list over DS973e; roster slot15 begins DS973a.
  for(uint32_t i=4;i<P;i++)p[15*P+i]=s[3420+i-4];
 }
 for(uint32_t i=selected;i<count;i++){
  for(uint32_t j=0;j<P;j++)p[i*P+j]=p[(i+1)*P+j];
  if(w(p+i*P+0x16)&0x8000)active=int(i);
 }
 sw(state,uint16_t(count-1));sw(state+2,uint16_t(active));return int(n);
}
extern "C" int32_t cc_career_close_theater(uint8_t* p,uint32_t selected){
 if(!p||p[0x23c]>20||selected>=p[0x23c])return -1;
 const uint32_t n=p[0x23c];const int id=p[0x23e + selected];
 if(p[0x23d]==selected)p[0x23d]=0;
 else if(p[0x23d]>selected)--p[0x23d];
 for(uint32_t i=selected+1;i<n;i++)p[0x23d+i]=p[0x23e + i];
 --p[0x23c];if(!p[0x23c])p[0x23d]=0x80;
 return id;
}
extern "C" int32_t cc_career_unlink_theater_nodes(uint8_t* ds,uint32_t h,uint32_t id,uint32_t all){
 if(!ds||h>65534||id>65535)return -1;
 uint16_t head=w(ds+h),node=head,prev=head;int removed=0;
 for(uint32_t steps=0;node;steps++){
  if(steps>=1638||node>65536-38)return -1;
  const uint16_t next=w(ds+node+0x24);
  if(w(ds+node+0x22)==id){
   if(node==head){head=next;sw(ds+h,head);}else sw(ds+prev+0x24,next);
   ++removed;if(!all)break;
  }else prev=node;
  node=next;
 }
 return removed;
}
