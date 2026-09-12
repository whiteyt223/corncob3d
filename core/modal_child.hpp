#pragma once
#include <stdint.h>
extern "C" {
// Host boundary translation of actual runtower after cc_modal_tower_audio.
// Begin formats the existing DS command once; does not write results/audio.
// Stages: 0 idle,1 calibrate,2 flush tiles,3 draw near only,4 clean objects,
// 5 clear bullets,6 write world,7 execute child,8 shareware score notice,
// 9 shareware TU notice,10 read STARTLOC,11 return to dotower.
void cc_modal_child_reset();
uint32_t cc_modal_child_begin();
uint32_t cc_modal_child_stage();
uint32_t cc_modal_child_advance(); // acknowledge stages2..6,8..10 only
// kind:0 tower,1 score,2 TU,3 calibration. Preserve the boss byte separately;
// both F1 and F2 execute tower.exe with their original different arguments.
uint32_t cc_modal_child_kind();
const char* cc_modal_child_program(); // lowercase original filename, empty if none
uint32_t cc_modal_child_command(); // canonical DS offset;0 when not at stage7
// Original child failure (carry or AL nonzero) sets error61. Successful return
// reaches read STARTLOC only when intowerflag!=0 and ejectflag==0.
uint32_t cc_modal_child_exec_result(uint32_t carry,uint32_t al);
// F5's far calibration returns error93 as a cancelled successful modal; every
// other source error is retained. rejoyflag is cleared on every return.
uint32_t cc_modal_child_calibration_result(uint32_t source_error);
}
