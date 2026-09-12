#include "edition.hpp"
namespace { uint32_t current_edition=0; }
extern "C" uint32_t cc_edition_set(uint32_t edition){
    if(edition>2)return 1;
    current_edition=edition;return 0;
}
extern "C" uint32_t cc_edition_get(){return current_edition;}
extern "C" uint32_t cc_edition_is_deluxe(){return current_edition==1;}
extern "C" uint32_t cc_edition_is_other_worlds(){return current_edition==2;}
