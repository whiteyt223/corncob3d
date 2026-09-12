// Regression: Other Worlds' timeout passed elapsed PIT ticks into voiceoff.
// Missing ADL.ASM's AND AL,DFh turned the screech voice ON every ~3.5 seconds.
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
const {instance:{exports:e}}=await WebAssembly.instantiate(await readFile(new URL('../dist/corncob.wasm',import.meta.url)),{});
const registers=()=>new Uint8Array(e.memory.buffer,e.cc_audio_register_ptr(),256);
for(let ax=0;ax<65536;ax++){
    e.cc_audio_voice_on(3,0x300);
    e.cc_audio_voice_off(3,ax);
    assert.equal(registers()[0xb3]&0x20,0,`voiceoff re-enabled channel 3 for AX=${ax.toString(16)}`);
}
for(const resetSound of [false,true]){
    e.cc_edition_set(2);e.cc_audio_reset();e.cc_audio_register_init();e.cc_audio_init_voices();
    // Two full 16-bit timer wraps, including a user toggling sound off/on.
    for(let tick=1;tick<=131072;tick++){
        if(resetSound&&tick===4096){e.cc_audio_sound_off();e.cc_audio_sound_on(0);}
        e.cc_audio_clock_advance(1);e.cc_audio_timer_tick();
        assert.equal(registers()[0xb3]&0x20,0,`idle timeout sounded channel 3 at tick ${tick}`);
    }
}
e.cc_audio_landing_screech();assert.ok(registers()[0xb3]&0x20,'landing screech still starts');
for(let tick=0;tick<256;tick++){e.cc_audio_clock_advance(1);e.cc_audio_timer_tick();}
assert.equal(registers()[0xb3]&0x20,0,'landing screech stops after timeout');
e.cc_audio_stall(1);assert.ok(registers()[0xb5]&0x20,'real stall warning still starts');
e.cc_audio_stall(0);assert.equal(registers()[0xb5]&0x20,0,'stall warning stops');
console.log('PASS voiceoff: all 65,536 AX values; idle timers across wraps and sound toggle; landing and stall sounds');
