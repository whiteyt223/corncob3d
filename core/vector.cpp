#include <stdint.h>

// F3DVEC.ASM: hmul/madd macros, dot, dot2, matvmul, mat2mul,
// calcmat and ncalcmat. Values are 16-bit words, not floating-point.
namespace {
int32_t signed16(uint32_t x) { x &= 65535u; return x < 32768u ? int32_t(x) : int32_t(x)-65536; }
uint16_t neg(uint16_t x) { return uint16_t(0u-x); }
uint16_t half(uint16_t x) { return uint16_t((x>>1)|(x&32768u)); }
uint16_t hmul(uint16_t a,uint16_t b) {
    const uint32_t p=uint32_t(signed16(a)*signed16(b));
    // Original special branch for DX=8000h is unreachable for signed 16x16.
    return uint16_t(p>>15);
}
uint16_t madd(uint16_t a,uint16_t b) {
    uint16_t sum=uint16_t(uint32_t(half(a))+half(b));
    // The assembly uses JS/JNS after CMP, i.e. the subtraction sign flag,
    // not the signed-comparison condition (SF xor OF).
    if (!(uint16_t(sum-16383u)&32768u)) sum=16383;
    else if (uint16_t(sum+16383u)&32768u) sum=uint16_t(-16383);
    return uint16_t((uint32_t(sum)<<1)|(sum>>15));
}
uint16_t dot(const uint16_t* a,const uint16_t* b) {
    return madd(madd(hmul(a[0],b[0]),hmul(a[1],b[1])),hmul(a[2],b[2]));
}
uint16_t dot2(const uint16_t* a,const uint16_t* b) {
    uint32_t p=0;
    for (unsigned i=0;i<3;++i) p+=uint32_t(signed16(a[3*i])*signed16(b[i]));
    int32_t hi=signed16(p>>16);
    if (hi>=16383) hi=16382;
    if (hi<=-16383) hi=-16382;
    return uint16_t((uint32_t(hi)<<1)|((p>>15)&1u));
}
uint16_t input[18],output[9];
}
extern "C" uint16_t* cc_vector_input() { return input; }
extern "C" uint16_t* cc_vector_output() { return output; }
extern "C" void cc_matvmul(const uint16_t* matrix,const uint16_t* vector,uint16_t* out) {
    for(unsigned row=0;row<3;++row) out[row]=dot(vector,matrix+3*row);
}
// F3DVEC.ASM primdot/pmvmul. drpoly's fast transform inlines this
// high-word rotate/add sequence; it differs from the saturating matvmul.
extern "C" void cc_pmvmul(const uint16_t* matrix,const uint16_t* vector,uint16_t* out) {
    for(unsigned row=0;row<3;++row){
        uint32_t sum=0;
        for(unsigned col=0;col<3;++col){
            const uint16_t high=uint16_t(uint32_t(signed16(matrix[row*3+col])*signed16(vector[col]))>>16);
            sum+=uint16_t((uint32_t(high)<<1)|(high>>15));
        }
        out[row]=uint16_t(sum);
    }
}
extern "C" void cc_mat2mul(const uint16_t* matrix,const uint16_t* column,uint16_t* out) {
    for(unsigned row=0;row<3;++row) out[3*row]=dot2(column,matrix+3*row);
}
// trig order is sya,cya,spa,cpa,sra,cra, exactly as written by calcsin.
extern "C" void cc_calcmat(const uint16_t* t,uint16_t* o) {
    const auto sy=t[0],cy=t[1],sp=t[2],cp=t[3],sr=t[4],cr=t[5];
    o[0]=hmul(cy,cp); o[1]=hmul(cp,sy); o[2]=neg(sp);
    o[3]=madd(hmul(hmul(sr,sp),cy),neg(hmul(cr,sy)));
    o[4]=madd(hmul(hmul(sr,sp),sy),hmul(cr,cy)); o[5]=hmul(cp,sr);
    o[6]=madd(hmul(hmul(cr,sp),cy),hmul(sr,sy));
    o[7]=madd(hmul(hmul(cr,sp),sy),neg(hmul(sr,cy))); o[8]=hmul(cp,cr);
}
extern "C" void cc_ncalcmat(const uint16_t* t,uint16_t* o) {
    const auto sy=neg(t[0]),cy=t[1],sp=neg(t[2]),cp=t[3],sr=neg(t[4]),cr=t[5];
    o[0]=hmul(cy,cp);
    auto cx=hmul(cy,sp);
    o[1]=madd(hmul(cx,sr),hmul(sy,cr));
    o[2]=madd(hmul(sy,sr),neg(hmul(cx,cr)));
    o[3]=neg(hmul(sy,cp)); cx=hmul(sy,sp);
    o[4]=madd(hmul(cy,cr),neg(hmul(cx,sr)));
    o[5]=madd(hmul(cx,cr),hmul(cy,sr));
    o[6]=sp; o[7]=neg(hmul(cp,sr)); o[8]=hmul(cp,cr);
}
extern "C" void cc_vector_run(uint32_t mode) {
    for (auto &v:output) v=0;
    if(mode==0) cc_matvmul(input,input+9,output);
    if(mode==1) cc_mat2mul(input,input+9,output);
    if(mode==2) cc_calcmat(input,output);
    if(mode==3) cc_ncalcmat(input,output);
    if(mode==4) cc_pmvmul(input,input+9,output);
}
