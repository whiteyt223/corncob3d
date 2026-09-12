#include <stdint.h>
// Browser byte-record staging, separate from original packed simulation DS.
namespace {alignas(8) uint8_t workspace[65536];}
extern "C" uint8_t* cc_browser_workspace(){return workspace;}
