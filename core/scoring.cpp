#include "scoring.hpp"
namespace {
int32_t word(int64_t n){const uint16_t u=uint16_t(n);return u<32768?int32_t(u):int32_t(u)-65536;}
int32_t dword(int64_t n){const uint32_t u=uint32_t(n);return u<0x80000000u?int32_t(u):int32_t(int64_t(u)-4294967296ll);}
uint16_t r16(const uint8_t* p,uint32_t o){return uint16_t(uint16_t(p[o])|(uint16_t(p[o+1])<<8));}
uint32_t r32(const uint8_t* p,uint32_t o){return uint32_t(r16(p,o))|(uint32_t(r16(p,o+2))<<16);}
int32_t sw(const uint8_t* p,uint32_t o){return word(r16(p,o));}
int32_t sd(const uint8_t* p,uint32_t o){return dword(r32(p,o));}
void w16(uint8_t* p,uint32_t o,int64_t v){const uint16_t u=uint16_t(v);p[o]=uint8_t(u);p[o+1]=uint8_t(u>>8);}
void w32(uint8_t* p,uint32_t o,int64_t v){const uint32_t u=uint32_t(v);w16(p,o,u);w16(p,o+2,u>>16);}
void copy(uint8_t* dst,const uint8_t* src,uint32_t n){for(uint32_t i=0;i<n;++i)dst[i]=src[i];}
bool name_valid(const uint8_t* p){for(unsigned i=0;i<28;++i)if(!p[i])return true;return false;}
bool same_name(const uint8_t* a,const uint8_t* b){for(unsigned i=0;i<28;++i){if(a[i]!=b[i])return false;if(!a[i])return true;}return false;}
void decode(const uint8_t* src,CCScoreFields& dst){for(unsigned i=0;i<65;++i)dst.words[i]=r16(src,2*i);}
void encode(const CCScoreFields& src,uint8_t* dst){for(unsigned i=0;i<65;++i)w16(dst,2*i,src.words[i]);}
void update_counter(uint8_t* pilot,unsigned offset){
    const int32_t value=sw(pilot,0x1c + offset);
    if(value>sw(pilot,0x128 + offset))w16(pilot,0x128 + offset,value);
    w16(pilot,0x1ae + offset,sw(pilot,0x1ae + offset)+value);
}
void award(uint8_t* pilot,CCScoreProgress& out,unsigned index){out.awards[index]=1;pilot[0x257 + index]=uint8_t(pilot[0x257 + index]+1);}
alignas(8) uint8_t workspace[CC_SCORE_WORKSPACE_BYTES];
}
extern "C" uint32_t cc_score_decode_result(const uint8_t* bytes,uint32_t size,CCScoreFields* fields){
    if(!bytes||!fields)return CC_SCORE_INVALID_ARGUMENT;
    if(size<CC_SCORE_INPUT_BYTES)return CC_SCORE_SHORT_BUFFER;
    CCScoreFields temp;decode(bytes,temp);*fields=temp;return CC_SCORE_OK;
}
extern "C" uint32_t cc_score_encode_result(const CCScoreFields* fields,uint8_t* bytes,uint32_t capacity){
    if(!fields||!bytes)return CC_SCORE_INVALID_ARGUMENT;
    if(capacity<CC_SCORE_INPUT_BYTES)return CC_SCORE_SHORT_BUFFER;
    const CCScoreFields temp=*fields;encode(temp,bytes);return CC_SCORE_OK;
}
extern "C" uint32_t cc_score_decode_file(const uint8_t* bytes,uint32_t size,CCScoreFile* file){
    if(!bytes||!file)return CC_SCORE_INVALID_ARGUMENT;
    if(size<CC_SCORE_FILE_BYTES)return CC_SCORE_SHORT_BUFFER;
    CCScoreFile temp{};decode(bytes,temp.fields);temp.checksum=r32(bytes,0x82);
    temp.error_code=r16(bytes,0x86);temp.scan_code=bytes[0x88];temp.scan_flag=bytes[0x89];temp.hash_pointer=r16(bytes,0x8a);
    *file=temp;return CC_SCORE_OK;
}
extern "C" uint32_t cc_score_encode_file(const CCScoreFile* file,uint8_t* bytes,uint32_t capacity){
    if(!file||!bytes)return CC_SCORE_INVALID_ARGUMENT;
    if(capacity<CC_SCORE_FILE_BYTES)return CC_SCORE_SHORT_BUFFER;
    const CCScoreFile temp=*file;encode(temp.fields,bytes);w32(bytes,0x82,temp.checksum);
    w16(bytes,0x86,temp.error_code);bytes[0x88]=temp.scan_code;bytes[0x89]=temp.scan_flag;w16(bytes,0x8a,temp.hash_pointer);return CC_SCORE_OK;
}
extern "C" uint32_t cc_score_encode_scored(const CCScoreFields* fields,int32_t score,uint8_t* bytes,uint32_t capacity){
    if(!fields||!bytes)return CC_SCORE_INVALID_ARGUMENT;
    if(capacity<CC_SCORE_SCORED_BYTES)return CC_SCORE_SHORT_BUFFER;
    const CCScoreFields temp=*fields;encode(temp,bytes);w32(bytes,0x82,score);return CC_SCORE_OK;
}
extern "C" uint32_t cc_score_compute(const uint8_t* result,uint32_t size,uint32_t mode,CCScoreBreakdown* output){
    if(!result||!output||mode>CC_SCORE_ISCORE_PREVIEW)return CC_SCORE_INVALID_ARGUMENT;
    if(size<CC_SCORE_INPUT_BYTES)return CC_SCORE_SHORT_BUFFER;
    CCScoreBreakdown value{};uint16_t status=r16(result,0);
    if(mode==CC_SCORE_ISCORE_PREVIEW)status=uint16_t((status|0x14u)&0xfffeu);
    // Original abort path returns before updating its factor globals. Expose
    // factors_valid=0 instead of inventing values for these stale globals.
    if(status&0x4000){*output=value;return CC_SCORE_OK;}
    if(mode==CC_SCORE_FINAL){
        value.pmsf=status&2?2:((status&0x8000)&&(status&0x100))?5:status&8?1:status&16?5:status&256?4:status&0x8000?2:1;
        // MOAG's ejected branch contains TEST [si],0 and is unreachable.
        value.msf=status&64?1:status&512?3:status&4?5:status&0x8000?5:1;
    }else{
        value.pmsf=status&2?2:status&8?1:status&16?5:status&256?4:1;
        value.msf=status&64?1:status&32?1:status&512?3:status&4?5:1;
    }
    int32_t base=word(15*sw(result,0x48)+sw(result,6)),prowess=10;
    for(unsigned i=0;i<8;++i){
        base=dword(int64_t(base)+word(sw(result,0x18 + 2*i)+sw(result,0x38 + 2*i)+sw(result,0x72 + 2*i)));
        prowess=word(prowess+word((i+1)*int64_t(sw(result,0x4e + 2*i))+(i+1)*int64_t(sw(result,0x5e + 2*i))));
    }
    int32_t mobf=dword(int64_t(dword(int64_t(base)*prowess))*5)/10;
    for(unsigned i=0;i<8;++i)mobf=dword(int64_t(mobf)+word((i+1)*int64_t(sw(result,8+2*i))+(i+1)*int64_t(sw(result,0x28 + 2*i))));
    const int32_t divisor=word(sw(result,4)+1);
    if(!divisor)return CC_SCORE_DIVIDE_BY_ZERO;
    const int32_t product=dword(int64_t(dword(int64_t(mobf)*value.pmsf))*value.msf);
    value.score=dword(int64_t(product)/divisor);value.mobf=mobf;value.factors_valid=1;*output=value;return CC_SCORE_OK;
}
extern "C" uint32_t cc_score_accumulate_mode(uint32_t mode,uint8_t* pilot,uint32_t pilot_size,const uint8_t* result,uint32_t result_size,uint8_t* scored,uint32_t capacity,CCScoreBreakdown* output){
    if(!pilot||!result||!scored)return CC_SCORE_INVALID_ARGUMENT;
    if(pilot_size<CC_SCORE_PILOT_BYTES||result_size<CC_SCORE_INPUT_BYTES||capacity<CC_SCORE_SCORED_BYTES)return CC_SCORE_SHORT_BUFFER;
    CCScoreBreakdown score;const uint32_t status=cc_score_compute(result,result_size,mode,&score);
    if(status)return status;
    uint8_t current[CC_SCORE_SCORED_BYTES];copy(current,result,CC_SCORE_INPUT_BYTES);w32(current,0x82,score.score);
    w16(pilot,0x18,sw(pilot,0x18)+1);copy(pilot+0x1c,current,CC_SCORE_SCORED_BYTES);
    if(sd(pilot,0x9e)>sd(pilot,0x124))copy(pilot+0xa2,current,CC_SCORE_SCORED_BYTES);
    w32(pilot,0x230,int64_t(sd(pilot,0x230))+sd(pilot,0x9e));
    constexpr unsigned singles[]={0x48u,6u,4u,0x4au,0x4cu};
    constexpr unsigned arrays[]={0x18u,0x38u,0x72u,8u,0x28u};
    for(unsigned offset:singles)update_counter(pilot,offset);
    for(unsigned base:arrays)for(unsigned i=0;i<8;++i)update_counter(pilot,base+2*i);
    if(sd(pilot,0x8a)>sd(pilot,0x196))w32(pilot,0x196,sd(pilot,0x8a));
    w32(pilot,0x21c,int64_t(sd(pilot,0x21c))+sd(pilot,0x8a));
    copy(scored,current,CC_SCORE_SCORED_BYTES);if(output)*output=score;return CC_SCORE_OK;
}
extern "C" uint32_t cc_score_accumulate(uint8_t* pilot,uint32_t pilot_size,const uint8_t* result,uint32_t result_size,uint8_t* scored,uint32_t capacity,CCScoreBreakdown* output){
    return cc_score_accumulate_mode(CC_SCORE_FINAL,pilot,pilot_size,result,result_size,scored,capacity,output);
}
extern "C" uint32_t cc_score_progress_mode(uint32_t other_worlds,uint8_t* pilot,uint32_t size,CCScoreTheater* theaters,uint32_t count,const CCScoreDefinition* definitions,uint32_t definition_count,int32_t theater_planes_lost,CCScoreProgress* output){
    if(!pilot||!output||(!theaters&&count)||(!definitions&&definition_count))return CC_SCORE_INVALID_ARGUMENT;
    if(size<CC_SCORE_PILOT_BYTES)return CC_SCORE_SHORT_BUFFER;
    if(count>CC_SCORE_MAX_OPEN_THEATERS||count!=pilot[0x23c]||definition_count>32767)return CC_SCORE_INVALID_THEATER_CONTEXT;
    for(unsigned i=0;i<count;++i)if(!name_valid(theaters[i].name))return CC_SCORE_INVALID_THEATER_CONTEXT;
    for(unsigned i=0;i<definition_count;++i)if(!name_valid(definitions[i].name))return CC_SCORE_INVALID_THEATER_CONTEXT;
    if(!(r16(pilot,0x16)&10)&&sw(pilot,0x66))return CC_SCORE_UNSUPPORTED_STOCKADE_CLOCK;
    CCScoreProgress out{};int completed=0,eligible=0,large_theaters=0;bool new_current=false;
    for(unsigned d=0;d<definition_count;++d){
        bool full=false;int most_destroyed=-1;
        for(unsigned i=0;i<count;++i){
            if(!same_name(definitions[d].name,theaters[i].name))continue;
            if(theaters[i].destroyed>most_destroyed)most_destroyed=theaters[i].destroyed;
            if(!theaters[i].remaining&&theaters[i].destroyed>0){
                full=true;
                if(i==pilot[0x23d]&&!theaters[i].awarded){new_current=true;theaters[i].awarded=1;out.theater_dirty=1;}
            }
        }
        if(definitions[d].eligible){++eligible;if(full)++completed;}
        if(most_destroyed>=20)++large_theaters;
    }
    unsigned desired=other_worlds?(large_theaters>=3?5:large_theaters>=2?4:sw(pilot,0x1f6)>=20?3:sw(pilot,0x1f6)>=7?2:sw(pilot,0x1f6)>=1?1:0):
        (r16(pilot,0x1c)&0x400)?3:sw(pilot,0x1f6)>=7?2:sw(pilot,0x1f6)>=1?1:0;
    if(!other_worlds&&(r16(pilot,0x1c)&0x400)&&pilot[0x256]!=3)out.special_promotion=1;
    if(new_current){if(!word(theater_planes_lost))award(pilot,out,8);award(pilot,out,9);}
    if(completed==eligible&&eligible>=3){
        if(other_worlds){desired=6;if(pilot[0x256]!=6&&!(r16(pilot,0x16)&2))out.special_promotion=1;}
        if(!(r16(pilot,0x16)&2)){pilot[0x25e]=1;out.awards[7]=1;}
    }
    if(pilot[0x256]<desired){out.promotion=uint16_t(desired-pilot[0x256]);pilot[0x256]=uint8_t(desired);}
    if(sw(pilot,0x68)>=27&&(r16(pilot,0x1c)&16)&&!sw(pilot,0x20)&&!(r16(pilot,0x1c)&64))award(pilot,out,0);
    const int32_t score=sd(pilot,0x9e);
    if(score>100000)award(pilot,out,6);else if(score>50000)award(pilot,out,3);else if(score>20000)award(pilot,out,2);else if(score>10000)award(pilot,out,1);
    if(sw(pilot,0x64)>=5)award(pilot,out,4);
    if(word((pilot[0x25c]+1)*10)<=sw(pilot,0x18)&&!(r16(pilot,0x1c)&2))award(pilot,out,5);
    *output=out;return CC_SCORE_OK;
}
extern "C" uint32_t cc_score_commit_sortie(){return CC_SCORE_UNSUPPORTED_EXTERNAL_COMMIT;}
extern "C" uint8_t* cc_score_workspace(){return workspace;}
extern "C" uint32_t cc_score_workspace_size(){return CC_SCORE_WORKSPACE_BYTES;}

extern "C" uint32_t cc_score_progress(uint8_t* p,uint32_t n,CCScoreTheater* t,uint32_t c,const CCScoreDefinition* d,uint32_t dc,int32_t lost,CCScoreProgress* out){return cc_score_progress_mode(0,p,n,t,c,d,dc,lost,out);}
