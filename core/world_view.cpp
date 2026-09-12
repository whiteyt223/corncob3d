#include "fixed.hpp"
#include "state.hpp"
using namespace cc;
extern "C" {
void cc_angles_matrix(const uint16_t*,uint16_t*,uint32_t);
void cc_matvmul(const uint16_t*,const uint16_t*,uint16_t*);
void cc_matrix_multiply(const uint16_t*,const uint16_t*,uint16_t*);
uint16_t* cc_scene_matrix();uint16_t* cc_scene_origin();uint32_t cc_scene_octant();
}
extern "C" {uint32_t* cc_world_input();uint32_t* cc_world_output();uint16_t* cc_world_local_matrix();}
// Source distance/LOD and drrobj using live DS observer/matrices. Existing
// world_input words6..15 supply object XYZ/angles, thresholds and cutoff;
// cached camera inputs0..5 are intentionally ignored. Output:
// distance code, rdf, unsigned squared distance, octant, mesh index.
// Local tmat deliberately persists when an object's rotation is skipped.
extern "C" uint32_t cc_prepare_object_view(uint32_t object_flags){
    auto* input=cc_world_input();auto* output=cc_world_output();auto* local=cc_world_local_matrix();
    if(!uint16_t(object_flags))return 1;
    int32_t delta[3];uint32_t bits=0;
    for(unsigned i=0;i<3;++i){delta[i]=d(input[6+i]-uint32_t(cc::state::l(0xb05+i*4)));uint32_t magnitude=delta[i]<0?0u-uint32_t(delta[i]):uint32_t(delta[i]);bits|=uint16_t(magnitude>>15);}
    {
        using namespace cc::state;
        unsigned z0=u(0x1efa),below=uint16_t(-w(uint32_t(delta[2])>>16));
        if(z0&&!(below&32768)&&below>z0){
            uint32_t magnitude=delta[2]<0?0u-uint32_t(delta[2]):uint32_t(delta[2]);
            unsigned height=uint16_t(magnitude>>12),limit=uint16_t(z0<<4);
            if(height>limit){
                unsigned factor=((limit<<16)/height)>>1;sw(0x1efc,factor);bits=0;
                for(unsigned i=0;i<3;++i){uint64_t product=uint64_t(int64_t(delta[i])*w(factor));delta[i]=d(uint32_t((product>>16)<<1));bits|=absw(w(product>>32));}
            }
        }
        for(unsigned i=0;i<3;++i)sl(0xe7e + i*4,delta[i]);
    }
    unsigned code=(bits&0xff00)?3:(bits&0xf0)?2:bits?1:0;output[0]=code;
    if(code==3)return 1;
    unsigned shift=code*4;uint32_t distance=0;
    for(int32_t v:delta){v=w(sar(v,shift));distance+=uint32_t(v*v);}output[2]=distance;
    unsigned rdf=code==0?(distance<=input[12]?0:4):code==1?(distance<=input[13]?4:8):8;
    if(code==2&&distance>input[14])return 1;
    if(rdf==8&&cc::state::u(0x1efa)&&!(object_flags&256)&&cc::state::s(0xb0f)>cc::state::s(0x1efa))return 1;
    output[1]=rdf;output[4]=rdf/4;
    uint16_t camera[9],angles[3],relative[3],origin[3],combined[9];
    for(unsigned i=0;i<3;++i){relative[i]=uint16_t(sar(delta[i],rdf+1));}
    {unsigned p=cc::state::u(0x1ef6);for(unsigned i=0;i<9;++i)camera[i]=cc::state::u(p+i*2);}
    cc_matvmul(camera,relative,origin);
    if(((code-uint8_t(input[15]))&128u)!=0){
        for(unsigned i=0;i<3;++i)angles[i]=uint16_t(input[9+i]);
        cc_angles_matrix(angles,local,1);cc_matrix_multiply(camera,local,combined);
    }else for(unsigned i=0;i<9;++i)combined[i]=cc::state::u(0x1fde + i*2);
    auto* sm=cc_scene_matrix();auto* so=cc_scene_origin();
    for(unsigned i=0;i<9;++i)sm[i]=local[i];
    for(unsigned i=0;i<3;++i)so[i]=relative[i];
    output[3]=cc_scene_octant();
    for(unsigned i=0;i<9;++i)sm[i]=combined[i];
    for(unsigned i=0;i<3;++i)so[i]=origin[i];
    return 0;
}
