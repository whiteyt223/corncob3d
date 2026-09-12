#include "fixed.hpp"
using namespace cc;
extern "C" {
int32_t cc_ssin(uint32_t);int32_t cc_scos(uint32_t);uint32_t cc_atn2(uint32_t,uint32_t);
void cc_calcmat(const uint16_t*,uint16_t*);void cc_ncalcmat(const uint16_t*,uint16_t*);
void cc_mat2mul(const uint16_t*,const uint16_t*,uint16_t*);
}
extern "C" void cc_angles_matrix(const uint16_t* angles,uint16_t* matrix,uint32_t inverse){
    uint16_t trig[6];for(unsigned i=0;i<3;++i){trig[i*2]=uint16_t(cc_ssin(angles[i]));trig[i*2+1]=uint16_t(cc_scos(angles[i]));}
    if(inverse)cc_ncalcmat(trig,matrix);else cc_calcmat(trig,matrix);
}
extern "C" void cc_matrix_multiply(const uint16_t* a,const uint16_t* b,uint16_t* out){
    uint16_t tmp[9];for(unsigned col=0;col<3;++col)cc_mat2mul(a,b+col,tmp+col);
    for(unsigned i=0;i<9;++i)out[i]=tmp[i];
}
// MAT.ASM getang uses its own saturated ADD, distinct from F3DVEC madd.
extern "C" void cc_matrix_angles(const uint16_t* matrix,uint16_t* angles){
    int32_t a[9];for(unsigned i=0;i<9;++i)a[i]=w(matrix[i]);
    int32_t sy,cy;
    if(a[3]>=-256&&a[3]<=255&&a[0]>=-256&&a[0]<=255){
        angles[0]=0;angles[1]=uint16_t(a[6]<0?16384:-16384);sy=0;cy=32767;
    }else{
        angles[0]=uint16_t(cc_atn2(uint16_t(a[0]),uint16_t(a[3])));
        sy=cc_ssin(angles[0]);cy=cc_scos(angles[0]);
        int32_t horizontal=add_sat(hm(sy,a[3]),hm(cy,a[0]));
        angles[1]=uint16_t(0u-cc_atn2(uint16_t(horizontal),uint16_t(a[6])));
    }
    const int32_t x=add_sat(hm(a[1],cy),hm(a[4],sy));
    const int32_t y=add_sat(w(-hm(a[1],sy)),hm(a[4],cy));
    const int32_t z=add_sat(hm(cc_scos(angles[1]),a[7]),hm(cc_ssin(angles[1]),x));
    angles[2]=uint16_t(cc_atn2(uint16_t(y),uint16_t(z)));
}
extern "C" void cc_rotate_plane(uint16_t* matrix,int32_t roll,int32_t pitch,int32_t yaw){
    const uint16_t angles[3]={uint16_t(yaw),uint16_t(pitch),uint16_t(roll)};uint16_t rotation[9];
    cc_angles_matrix(angles,rotation,1);cc_matrix_multiply(matrix,rotation,matrix);
}
