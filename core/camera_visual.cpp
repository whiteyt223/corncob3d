#include "camera_visual.hpp"
#include "state.hpp"
using namespace cc;using namespace cc::state;
extern "C" {
void cc_angles_matrix(const uint16_t*,uint16_t*,uint32_t);
void cc_matvmul(const uint16_t*,const uint16_t*,uint16_t*);
void cc_matrix_multiply(const uint16_t*,const uint16_t*,uint16_t*);
uint16_t* cc_scene_matrix();uint16_t* cc_scene_origin();uint16_t* cc_world_local_matrix();uint32_t cc_scene_octant();
uint8_t* cc_framebuffer();uint8_t* cc_video_memory();uint32_t* cc_runtime_state();void cc_audio_stop_blam();
}
extern "C" void cc_camera_erase_front(){
    wb(0x1b8b,0);
    unsigned window=u(0x1c3e),start=u(window+20),width=((unsigned(u(window+8))+1)>>4)*16,rows=u(window+10);
    // scrfbx writes words, so the152pixel gwinl clears144pixels per row.
    for(unsigned y=0;y<rows;++y)for(unsigned x=0;x<width;++x){unsigned p=(uint16_t(start+y*80)*8)+x;if(p<640*350)cc_framebuffer()[p]=0;}
}
extern "C" uint32_t cc_camera_remote_prepare(){
    uint16_t pilot_inverse[9],forward[9],angles[3],local[9],relative[3],origin[3],combined[9],fixed[3]={7000,0,0};
    unsigned pointer=u(0x1ef6),mesh=u(0x5bb0+40);
    for(unsigned i=0;i<9;++i){pilot_inverse[i]=u(0x1ff0+i*2);forward[i]=u(pointer+i*2);}
    for(unsigned i=0;i<3;++i){angles[i]=u(0x5bb0+12+i*2);sw(0x1d0c+i*2,fixed[i]);}
    cc_matvmul(pilot_inverse,fixed,relative);cc_angles_matrix(angles,local,1);
    cc_matvmul(forward,relative,origin);cc_matrix_multiply(forward,local,combined);
    wb(0x205d,0);sw(0x205f,mesh);
    for(unsigned i=0;i<9;++i){sw(0x2014+i*2,local[i]);sw(0x2002+i*2,combined[i]);cc_world_local_matrix()[i]=local[i];cc_scene_matrix()[i]=local[i];}
    for(unsigned i=0;i<3;++i){sw(0x1d06+i*2,relative[i]);sw(mesh+i*2,origin[i]);cc_scene_origin()[i]=relative[i];}
    unsigned octant=cc_scene_octant();sw(0x2058,octant);
    for(unsigned i=0;i<9;++i)cc_scene_matrix()[i]=combined[i];
    for(unsigned i=0;i<3;++i)cc_scene_origin()[i]=origin[i];
    return octant;
}

namespace {unsigned display_address=0;}
extern "C" uint32_t cc_camera_display_address(){return display_address;}
extern "C" uint32_t cc_camera_flip_page(){
    if(u(0xe43)){wb(0x2158,b(0x2158)^11);sw(0xe43,u(0xe43)-1);}
    unsigned shift=0;
    if(b(0x19f3)){if(!b(0xafc))shift=(b(0x1bc8)&1)?80:0;}
    else if(b(0xafc)){wb(0x2158,7);sw(0xe43,0);wb(0x1a0f,0);}
    else if(b(0x1a0f)){wb(0x1a0f,b(0x1a0f)-1);shift=(b(0x1bc8)&1)?241:0;}
    if(!b(0x1a0f)&&u(0x1da)==65535){cc_audio_stop_blam();sw(0x1da,cc_runtime_state()[0]);}
    wb(0xd5d,b(0xd5d)^255);
    unsigned next=b(0xd5d)!=0;sw(0x1b90,next?0:0x7e00);display_address=(next?0x7e00:0)+shift;
    return next;
}
extern "C" uint32_t cc_camera_rear_input(){
    if(b(0x26c)!=0x93||u(0x1c3e)!=0x1bf6)return 0;
    wb(0x26c,0);
    if(!u(0x1c40)){sw(0x1c40,0x1c0e);return 1;}
    auto* video=cc_video_memory();
    for(unsigned pass=0;pass<2;++pass){
        wb(0x1b8b,0);unsigned start=uint16_t(u(0x1b90)+20341);
        for(unsigned y=0;y<90;++y)for(unsigned byte=0;byte<38;++byte)for(unsigned bit=0;bit<8;++bit)video[uint16_t(start+y*80+byte)*8+bit]=0;
        cc_camera_flip_page();
    }
    sw(0x1c40,0);return 2;
}

extern "C" void cc_camera_present(){
    auto* frame=cc_framebuffer();auto* video=cc_video_memory();
    for(unsigned i=0;i<640*350;++i)frame[i]=video[uint16_t(display_address+i/8)*8+(i&7)];
}
