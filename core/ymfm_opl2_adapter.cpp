// ymfm BSD-3-Clause core bridge. Source: aaronsgiles/ymfm, pinned in
// third_party/ymfm; see THIRD_PARTY.md for retrieval revision and license.
#include "../third_party/ymfm/ymfm_opl.h"
#include <new>
#include <cstddef>
#include <stdint.h>
namespace {
constexpr size_t HEAP_BYTES=65536;
alignas(std::max_align_t) uint8_t heap[HEAP_BYTES];
size_t heap_used=0;
}
void* operator new(std::size_t size) { size=(size+alignof(std::max_align_t)-1)&~(alignof(std::max_align_t)-1); if(heap_used+size>HEAP_BYTES)__builtin_trap();void* p=heap+heap_used;heap_used+=size;return p; }
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void*) noexcept {}
void operator delete[](void*) noexcept {}
void operator delete(void*,std::size_t) noexcept {}
void operator delete[](void*,std::size_t) noexcept {}
namespace {
class Interface final : public ymfm::ymfm_interface {};
Interface interface;
alignas(ymfm::ym3812) uint8_t chip_storage[sizeof(ymfm::ym3812)];
ymfm::ym3812* chip;
int16_t output_buffer[2048];
ymfm::ym3812& ensure(){if(!chip){chip=new(chip_storage) ymfm::ym3812(interface);}return *chip;}
}
extern "C" void cc_ym3812_initialize(){(void)ensure();}
extern "C" void cc_ym3812_reset(){ensure().reset();}
extern "C" void cc_ym3812_write(uint32_t reg,uint32_t value){ensure().write_address(uint8_t(reg));ensure().write_data(uint8_t(value));}
extern "C" uint32_t cc_ym3812_native_rate(){return ensure().sample_rate(3579545);}
extern "C" uint32_t cc_ym3812_buffer(){return uint32_t(uintptr_t(output_buffer));}
extern "C" void cc_ym3812_render(uint32_t count){if(count>2048)count=2048;for(uint32_t i=0;i<count;++i){ymfm::ym3812::output_data output;ensure().generate(&output);output_buffer[i]=int16_t(output.data[0]);}}
extern "C" int32_t cc_ym3812_sample_at(uint32_t at){return at<2048?output_buffer[at]:0;}
