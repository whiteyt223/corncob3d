#pragma once
#include <stdint.h>
// Source-derived OPL2 event stream. Times are monotonically increasing raw
// PIT ticks (1193182/1024 Hz); no browser clock enters the core.
namespace cc::audio {
enum class Preset : uint8_t { engine, gun, boom, screech, blam, snare, scrape, wind, stall, saucer, orb, kla };
void reset();
void advance(uint32_t raw_ticks);
void write(uint8_t reg,uint8_t value);
void register_init();
void init_voices();
void preset(Preset which);
void voice_on(uint8_t voice,uint16_t ax);
void voice_off(uint8_t voice,uint16_t ax);
void snare_sound(bool wind_active);
void big_boom();
// 3.ASM bulstrt invokes voiceoff before loading AX=0x200 for voiceon.
// Preserve the caller's residual AX explicitly; voiceoff consumes AH.
void shot(uint16_t off_ax);
void missile(bool wind_active);
void crash();
void crash_landing();
// Source-only leaves retained as explicit events so their existing owners can
// keep the original condition and DS mutation beside the audio write.
void stop_blam();                 // F3DVEC qstopblam: voiceoff(2), AX=0x10
void scrape_stop();               // 3.ASM crash settle: BD=0x20
void reset_missile_patch();       // 3.ASM repair path: snareinit
void tower();                     // 3.ASM runtower entry: stop 0/2/3, reset ambience
void collision_blam();
void explosion(uint16_t off_ax);
void stall(bool active);
// Legacy convenience wrapper only. Source call sites must emit engine writes inline
// so their AX residual and word-width control flow remain untouched.
void engine(bool audible,uint16_t currpmx,int16_t oil,int16_t speed,uint16_t engdam);
void sound_off();
void sound_on(bool ejected);
void pause();
void eject();
void landing_screech();
void timer_tick();
// 3.ASM:7640..7791. Inputs are the source frame values, not distances
// recomputed by audio. State mirrors only audio-private source words.
void ambient_frame(uint16_t orbdist,uint8_t orbfreq,uint16_t saucerdist,uint8_t ssfreq,uint16_t kladist,uint8_t klaqual);
bool ambient_active();
uint32_t register_ptr();
uint32_t editor_buffer();
void editor_reset();
uint32_t editor_commit(uint8_t index,uint8_t pairs);
}
extern "C" {
void cc_audio_reset();
void cc_audio_clock_advance(uint32_t raw_ticks);
void cc_audio_write(uint32_t reg,uint32_t value);
void cc_audio_register_init();
void cc_audio_init_voices();
void cc_audio_voice_on(uint32_t voice,uint32_t ax);
void cc_audio_voice_off(uint32_t voice,uint32_t ax);
void cc_audio_preset(uint32_t preset);
void cc_audio_snare(uint32_t wind_active);
void cc_audio_shot(uint32_t off_ax);
void cc_audio_missile(uint32_t wind_active);
void cc_audio_crash();
void cc_audio_crash_landing();
void cc_audio_stop_blam();
void cc_audio_scrape_stop();
void cc_audio_reset_missile_patch();
void cc_audio_tower();
void cc_audio_collision_blam();
void cc_audio_explosion(uint32_t off_ax);
void cc_audio_big_boom();
void cc_audio_stall(uint32_t active);
void cc_audio_engine(uint32_t audible,uint32_t currpmx,int32_t oil,int32_t speed,uint32_t engdam);
void cc_audio_sound_off();
void cc_audio_sound_on(uint32_t ejected);
void cc_audio_pause();
void cc_audio_eject();
void cc_audio_landing_screech();
void cc_audio_timer_tick();
void cc_audio_ambient_frame(uint32_t orbdist,uint32_t orbfreq,uint32_t saucerdist,uint32_t ssfreq,uint32_t kladist,uint32_t klaqual);
uint32_t cc_audio_ambient_active();
// Current OPL register image for bridge resynchronization. 256 bytes.
uint32_t cc_audio_register_ptr();
// MOAG Sounds Editor: 12 * (14 register/value pairs + ff) staging bytes.
uint32_t cc_audio_editor_buffer();
void cc_audio_editor_reset();
uint32_t cc_audio_editor_commit(uint32_t index,uint32_t pairs);
// Event record is {u32 sequence,u32 raw_tick,u8 register,u8 value,u16 pad}.
uint32_t cc_audio_event_ptr();
uint32_t cc_audio_event_capacity();
uint32_t cc_audio_event_sequence();
uint32_t cc_audio_raw_ticks();
uint32_t cc_audio_event_at(uint32_t sequence); // register | value<<8; 0xffffffff if overwritten
uint32_t cc_audio_event_tick_at(uint32_t sequence);
}
