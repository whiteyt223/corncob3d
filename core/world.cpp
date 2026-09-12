#include "fixed.hpp"
using namespace cc;
extern "C" {
void cc_angles_matrix(const uint16_t*,uint16_t*,uint32_t);
void cc_matvmul(const uint16_t*,const uint16_t*,uint16_t*);
void cc_matrix_multiply(const uint16_t*,const uint16_t*,uint16_t*);
uint16_t* cc_scene_matrix();uint16_t* cc_scene_origin();uint32_t cc_scene_octant();
}
namespace {uint32_t input[16],output[5];uint16_t local[9];}
extern "C" uint32_t* cc_world_input(){return input;}
extern "C" uint32_t* cc_world_output(){return output;}
extern "C" uint16_t* cc_world_local_matrix(){return local;}
// Source distance/LOD and drrobj preparation. Inputs: camera XYZ/angles,
// object XYZ/angles, three distance thresholds, rotation cutoff. Output:
// distance code, rdf, unsigned squared distance, octant, mesh index.
// Local tmat deliberately persists when an object's rotation is skipped.
extern "C" uint32_t cc_prepare_object(){
    int32_t delta[3];uint32_t bits=0;
    for(unsigned i=0;i<3;++i){delta[i]=d(input[6+i]-input[i]);uint32_t magnitude=delta[i]<0?0u-uint32_t(delta[i]):uint32_t(delta[i]);bits|=uint16_t(magnitude>>15);}
    unsigned code=(bits&0xff00)?3:(bits&0xf0)?2:bits?1:0;output[0]=code;
    if(code==3)return 1;
    unsigned shift=code*4;uint32_t distance=0;
    for(int32_t v:delta){v=w(sar(v,shift));distance+=uint32_t(v*v);}output[2]=distance;
    unsigned rdf=code==0?(distance<=input[12]?0:4):code==1?(distance<=input[13]?4:8):8;
    if(code==2&&distance>input[14])return 1;
    output[1]=rdf;output[4]=rdf/4;
    uint16_t camera[9],angles[3],relative[3],origin[3],combined[9];
    for(unsigned i=0;i<3;++i){angles[i]=uint16_t(input[3+i]);relative[i]=uint16_t(sar(delta[i],rdf+1));}
    cc_angles_matrix(angles,camera,0);cc_matvmul(camera,relative,origin);
    if(((code-uint8_t(input[15]))&128u)!=0){
        for(unsigned i=0;i<3;++i)angles[i]=uint16_t(input[9+i]);
        cc_angles_matrix(angles,local,1);cc_matrix_multiply(camera,local,combined);
    }else for(unsigned i=0;i<9;++i)combined[i]=camera[i];
    auto* sm=cc_scene_matrix();auto* so=cc_scene_origin();
    for(unsigned i=0;i<9;++i)sm[i]=local[i];
    for(unsigned i=0;i<3;++i)so[i]=relative[i];
    output[3]=cc_scene_octant();
    for(unsigned i=0;i<9;++i)sm[i]=combined[i];
    for(unsigned i=0;i<3;++i)so[i]=origin[i];
    return 0;
}
