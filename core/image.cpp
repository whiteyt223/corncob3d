#include <stdint.h>

// Source: RIMG.ASM wrtimg/imglp2. Decode its alternating fill/literal runs
// into the byte mask written to the EGA framebuffer. Hardware plane writes
// are deliberately separate. The original skips the first four file bytes.
extern "C" int32_t cc_decode_img(const uint8_t *input,uint32_t size,uint8_t *output,uint32_t target) {
    if (!input || !output || size<4 || size>16383 || target>28000) return -1;
    uint32_t cursor=4,written=0;
    while(written<target) {
        if(cursor+3>size) return -2;
        uint32_t count=(input[cursor]|(uint32_t(input[cursor+1])<<8))&0x3fff;
        cursor+=2;
        if(count>target-written)count=target-written;
        const uint8_t value=input[cursor++];
        for(uint32_t i=0;i<count;i++)output[written++]=value;
        // Original files can end on the final fill. RIMG reads an unused word
        // beyond EOF but clamps the subsequent literal count to zero.
        if(written==target)return static_cast<int32_t>(cursor);
        if(cursor+2>size)return -3;
        uint32_t literal=(input[cursor]|(uint32_t(input[cursor+1])<<8))&0x3fff;
        cursor+=2;
        if(literal>target-written)literal=target-written;
        if(cursor+literal>size)return -4;
        for(uint32_t i=0;i<literal;i++)output[written++]=input[cursor++];
        if(count==0 && literal==0)return -5;
    }
    return static_cast<int32_t>(cursor);
}

static uint8_t image_input[16384];
static uint8_t image_output[28000];
extern "C" uint8_t *cc_image_input(){return image_input;}
extern "C" uint8_t *cc_image_output(){return image_output;}
extern "C" int32_t cc_decode_image_buffer(uint32_t size,uint32_t target){
    return cc_decode_img(image_input,size,image_output,target);
}
