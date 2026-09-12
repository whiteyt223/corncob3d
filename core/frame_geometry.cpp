#include "state.hpp"
#include "runtime.hpp"
using namespace cc;
using namespace cc::state;
// Original 3.42 CS3DBC..3F05 (3.ASM freeze4a through fmszlp).
// Runs once per simulation frame, including frozen/ejected/remote states,
// after aircraft/pilot movement and before late weapons. Rendering consumes
// the resulting mutable geometry on the next frame. Exactly 141 RNG draws.
extern "C" void cc_frame_geometry(){
    wb(0x2dc2,b(0xf55)&&(raw_ticks()&1024)?15:0);
    for(unsigned p=0x2dc3;p<=0x2dc7;p+=2){unsigned saved=b(p);wb(p,b(p+1));wb(p+1,saved);}
    wb(0x31b6,random_bound(15));
    struct Sequence {unsigned source,destination,rows,bound,increment,bias;};
    const Sequence sequences[]={
        {0x379d,0x36fb,16,300,30,0},
        {0x3659,0x35b7,16,60,7,120},
        {0x3e74,0x3e54,3,500,62,1000}
    };
    for(const auto& sequence:sequences){
        unsigned source=sequence.source+2,destination=sequence.destination+2,bound=sequence.bound;
        for(unsigned row=0;row<sequence.rows;++row){
            for(unsigned column=0;column<4;++column){
                sw(destination,random_bound(bound)+sequence.bias+u(source));
                source+=2;destination+=2;
            }
            bound+=sequence.increment;source+=2;destination+=2;
        }
    }
}
