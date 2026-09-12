#pragma once
#include <stdint.h>
extern "C" {
// Source flushtile continues at current DS tileptr/tobjcount/curtile[x,y].
// Status0 completes,2 rejects malformed external storage/table inputs.
uint32_t cc_flush_tile();
uint32_t cc_flush_tiles();
// Source donewmiss/doxp:0 no request,1 transition completed,2 arithmetic or
// malformed-memory failure; donewmiss additionally returns3 for damage>=16.
uint32_t cc_donewmiss();
uint32_t cc_doxp();
// Diagnostic source call order, count then up to32 (kind,x,y) triples.
// kinds:1 farmmemseg,2 flushtile,3 empty drawnear,4 mission relocation,
// 5 portal relocation,6 bringmemseg. Reset at each top-level transition/flush.
uint32_t* cc_transition_events();
}
