#include "audio.hpp"
#include "edition.hpp"
#include <stddef.h>
namespace cc::audio {
namespace {
struct Event { uint32_t sequence, tick; uint8_t reg, value; uint16_t pad; };
constexpr uint32_t kCapacity=32768;
Event events[kCapacity];
uint32_t next_sequence=0, clock_ticks=0;
uint8_t registers[256]{};
// 3.ASM data words local to the sound driver.
uint8_t orb_on=0,saucer_on=0,saucer_active=0;
uint16_t old_kla_distance=0,real_kla_frequency=0,screech_time=0;
// MOAG Sounds Editor stores at most 14 register/value pairs plus ff per sound.
constexpr uint8_t kEditorTables=12,kEditorPairs=14,kEditorStride=kEditorPairs*2+1;
uint8_t editor_tables[kEditorTables][kEditorStride]{};uint16_t editor_mask=0;
// ADL.ASM:645 calls readsounds after adlreginit. These are the 12 tables
// loaded from the release 3.ADL, terminated exactly as tblinit expects.
constexpr uint8_t engine_table[]={0xa0,0x00,0xb0,0x0d,0xc0,0x00,0x20,0x21,0x23,0x22,0x40,0x00,0x43,0x09,0x60,0x70,0x63,0x70,0x80,0x05,0x83,0x07,0xe0,0x01,0xe3,0x01,0xff};
constexpr uint8_t gun[]={0xa1,0x00,0xb1,0x0d,0xc1,0x00,0x21,0x01,0x24,0x01,0x41,0x00,0x44,0x00,0x61,0x8f,0x64,0xff,0x81,0x08,0x84,0x08,0xe1,0x00,0xe4,0x03,0xff};
constexpr uint8_t boom[]={0xa2,0x00,0xb2,0x00,0xc2,0x00,0x22,0x01,0x25,0x01,0x42,0x00,0x45,0x09,0x62,0xf5,0x65,0xf5,0x82,0x05,0x85,0x03,0xe2,0x01,0xe5,0x00,0xff};
constexpr uint8_t screech[]={0xa3,0x00,0xb3,0x0d,0xc3,0x00,0x28,0x24,0x2b,0x25,0x48,0x00,0x4b,0x10,0x68,0xf8,0x6b,0xf0,0x88,0xff,0x8b,0xff,0xe8,0x03,0xeb,0x03,0xff};
constexpr uint8_t blam[]={0xa2,0x00,0xb2,0x0d,0xc2,0x00,0x22,0x01,0x25,0x21,0x42,0x00,0x45,0x00,0x62,0xf0,0x65,0xf0,0x82,0x03,0x85,0x05,0xe2,0x01,0xe5,0x00,0xff};
constexpr uint8_t snare[]={0xa7,0x20,0xb7,0x00,0xc7,0x00,0x34,0x03,0x54,0x20,0x74,0x64,0x94,0x62,0xf4,0x00,0xff};
// Table index 6, loaded by ADL.ASM readsounds. The verified Other Worlds
// 3.ADL changes this preset only; all other instrument tables are shared.
constexpr uint8_t scrape_shareware[]={0xa7,0x20,0xb7,0x00,0xc7,0x00,0x34,0x03,0x54,0xf4,0x74,0xf9,0x94,0x62,0xf4,0x00,0xff};
constexpr uint8_t scrape_other_worlds[]={0xa7,0x20,0xb7,0x00,0xc7,0x00,0x34,0x20,0x54,0x08,0x74,0xf2,0x94,0xcf,0xf4,0x00,0xff};
constexpr uint8_t wind[]={0xa7,0x20,0xb7,0x00,0xc7,0x00,0x34,0x03,0x54,0x30,0x74,0x10,0x94,0x01,0xf4,0x00,0xff};
constexpr uint8_t stall_table[]={0xa5,0x00,0xb5,0x0d,0xc5,0x01,0x2a,0x22,0x2d,0x2f,0x4a,0x18,0x4d,0x18,0x6a,0x80,0x6d,0x80,0x8a,0x09,0x8d,0x09,0xea,0x00,0xed,0x01,0xff};
constexpr uint8_t saucer[]={0xa3,0x00,0xb3,0x0d,0xc3,0x00,0x28,0x20,0x2b,0x28,0x48,0x00,0x4b,0x00,0x68,0xcf,0x6b,0xcf,0x88,0x0c,0x8b,0x0c,0xe8,0x00,0xeb,0x00,0xff};
constexpr uint8_t orb[]={0xa3,0x00,0xb3,0x0d,0xc3,0x01,0x28,0x26,0x2b,0x28,0x48,0x00,0x4b,0x00,0x68,0xff,0x6b,0xff,0x88,0x0f,0x8b,0x0f,0xe8,0x00,0xeb,0x00,0xff};
constexpr uint8_t kla[]={0xa4,0x00,0xb4,0x01,0xc4,0x00,0x29,0x21,0x2c,0x2f,0x49,0x00,0x4c,0x10,0x69,0x95,0x6c,0x95,0x89,0x02,0x8c,0x02,0xe9,0x00,0xec,0x00,0xff};
void table(const uint8_t* p){while(*p!=0xff){uint8_t r=*p++,v=*p++;write(r,v);}}
}
void reset(){next_sequence=0;clock_ticks=0;for(uint8_t& r:registers)r=0;orb_on=saucer_on=saucer_active=0;old_kla_distance=real_kla_frequency=screech_time=0;}
void advance(uint32_t ticks){clock_ticks+=ticks&0xffffu;}
void write(uint8_t reg,uint8_t value){registers[reg]=value;events[next_sequence%kCapacity]={next_sequence,clock_ticks,reg,value,0};++next_sequence;}
void register_init(){
    // ADL.ASM:314..379. Preserve its contiguous register ranges, including
    // writes to unused OPL slots; this is the observable original sequence.
    for(uint8_t r=0x40;r<0x53;++r)write(r,63);
    for(uint8_t r=0x53;r<0x60;++r)write(r,20);
    for(uint8_t r=0xb0;r<0xb8;++r)write(r,13);
    write(1,0);write(1,0x20);write(8,0);
    for(uint8_t r=0x20;r<0x40;++r)write(r,1);
    for(uint8_t r=0x60;r<0x80;++r)write(r,0xf3);
    for(uint8_t r=0x80;r<0xa0;++r)write(r,3);
    for(uint8_t r=0xa0;r<0xa8;++r)write(r,0);
    write(0xbd,0x20);
    for(uint8_t r=0xc0;r<0xc8;++r)write(r,0);
    for(uint8_t r=0xe0;r<0xf6;++r)write(r,0);
}
const uint8_t* built_in(Preset p){switch(p){
 case Preset::engine:return engine_table;case Preset::gun:return gun;case Preset::boom:return boom;
 case Preset::screech:return screech;case Preset::blam:return blam;case Preset::snare:return snare;
 case Preset::scrape:return cc_edition_is_other_worlds()?scrape_other_worlds:scrape_shareware;case Preset::wind:return wind;case Preset::stall:return stall_table;
 case Preset::saucer:return saucer;case Preset::orb:return orb;case Preset::kla:return kla;}return engine_table;}
void preset(Preset p){uint8_t index=uint8_t(p);table((editor_mask&(uint16_t(1)<<index))?editor_tables[index]:built_in(p));}
void voice_on(uint8_t voice,uint16_t ax){write(uint8_t(0xa0+voice),uint8_t(ax));write(uint8_t(0xb0+voice),uint8_t(((ax>>8)<<2)|0x21));}
void voice_off(uint8_t voice,uint16_t ax){write(uint8_t(0xb0+voice),uint8_t(((ax>>8)<<2)|1));}
void init_voices(){preset(Preset::snare);preset(Preset::gun);preset(Preset::boom);write(0xb6,1);write(0x50,0);write(0x53,0);preset(Preset::engine);preset(Preset::screech);preset(Preset::stall);preset(Preset::kla);}
void snare_sound(bool wind_active){if(!wind_active){write(0xbd,0x20);write(0xbd,0x28);}}
void big_boom(){write(0xbd,0x20);write(0xbd,0x30);}
void shot(uint16_t off_ax){voice_off(1,off_ax);voice_on(1,0x200);}
void missile(bool wind_active){snare_sound(wind_active);}
void crash(){preset(Preset::scrape);snare_sound(false);voice_on(4,0x10);voice_on(3,0x300);voice_off(2,0x300);preset(Preset::blam);voice_on(2,0x10);}
void crash_landing(){preset(Preset::scrape);snare_sound(false);voice_on(3,0x300);preset(Preset::blam);voice_off(2,0x10);voice_on(2,0x10);}
void stop_blam(){voice_off(2,0x10);}
void scrape_stop(){write(0xbd,0x20);}
void reset_missile_patch(){preset(Preset::snare);}
void tower(){voice_off(0,0x100);voice_off(2,0x10);voice_off(3,0x10);saucer_on=orb_on=0;}
void collision_blam(){preset(Preset::blam);voice_on(2,0x10);}
void explosion(uint16_t off_ax){voice_off(2,off_ax);voice_on(2,0);}
void stall(bool active){if(active)voice_on(5,0x180);else voice_off(5,0x200);}
uint32_t register_ptr(){return uint32_t(uintptr_t(registers));}
uint32_t editor_buffer(){return uint32_t(uintptr_t(editor_tables));}
void editor_reset(){editor_mask=0;}
uint32_t editor_commit(uint8_t index,uint8_t pairs){
    if(index>=kEditorTables||pairs==0||pairs>kEditorPairs)return 1;
    uint8_t* p=editor_tables[index];
    for(uint8_t i=0;i<pairs;++i)if(p[i*2]==0xff)return 1;
    p[pairs*2]=0xff;editor_mask|=uint16_t(1)<<index;return 0;
}
uint32_t event_ptr(){return uint32_t(uintptr_t(events));}
uint32_t event_sequence(){return next_sequence;}
uint32_t raw_ticks(){return clock_ticks;}
uint32_t event_at(uint32_t sequence){const Event& e=events[sequence%kCapacity];return e.sequence==sequence?uint32_t(e.reg)|(uint32_t(e.value)<<8):0xffffffffu;}
uint32_t event_tick_at(uint32_t sequence){const Event& e=events[sequence%kCapacity];return e.sequence==sequence?e.tick:0xffffffffu;}

void sound_off(){for(uint8_t voice=0;voice<7;++voice)voice_off(voice,0x100);register_init();}
void sound_on(bool ejected){voice_off(0,0x100);init_voices();voice_off(0,0x100);if(ejected)preset(Preset::wind);saucer_on=orb_on=0;old_kla_distance=64;}
void pause(){voice_off(0,0x100);voice_off(2,0x10);}
void eject(){voice_off(0,0x100);voice_off(2,0x10);voice_off(5,0x200);voice_off(2,0x200);voice_off(3,0x200);preset(Preset::wind);}
void landing_screech(){screech_time=uint16_t(clock_ticks);voice_on(3,0x300);}
void timer_tick(){
    // Native INT8 fixtures prove shareware has no timeout body. Other Worlds
    // executes it every 64 raw ticks after INC oticks.
    if(!cc_edition_is_other_worlds()||(clock_ticks&63u)||saucer_active||orb_on)return;
    uint16_t elapsed=uint16_t(uint16_t(clock_ticks)-screech_time);
    if(elapsed<200)return;
    voice_off(3,elapsed);saucer_on=orb_on=0;
}
void ambient_frame(uint16_t orbdist,uint8_t orbfreq,uint16_t saucerdist,uint8_t ssfreq,uint16_t kladist,uint8_t klaqual){
    uint16_t ax=orbdist;
    if(!orb_on){
        if(ax<63){voice_off(3,0xff);preset(Preset::orb);orb_on=0xff;voice_on(3,orbfreq);ax=orbdist;}
        else goto check_saucer;
    }
    if(ax<63){
        int16_t volume=int16_t(ax)-13;if(volume<0)volume=0;
        write(0x4b,uint8_t(volume));write(0x48,uint8_t(volume));write(0xa3,orbfreq);goto kla_sound;
    }
    voice_off(3,0x10);saucer_on=0;preset(Preset::screech);orb_on=0;
check_saucer:
    if(orb_on)goto kla_sound;
    saucer_active=0;
    if(saucerdist<255){
        saucer_active=0xff;
        if(!saucer_on){voice_off(3,saucerdist);preset(Preset::saucer);saucer_on=0xff;voice_on(3,0xff);}
        uint16_t magnitude=saucerdist>>2;write(0x4b,uint8_t(magnitude));write(0xa3,ssfreq);
    } else if(saucer_on) {voice_off(3,0x10);saucer_on=0;preset(Preset::screech);}
kla_sound:
    uint16_t desired=uint16_t((uint16_t(klaqual)<<4)+40);
    int16_t difference=int16_t(uint16_t(desired-real_kla_frequency));
    // x86 SAR twice: floor toward negative infinity, including odd negatives.
    int16_t adjustment=difference>=0?int16_t(difference/4):int16_t(-((int(-difference)+3)/4));
    real_kla_frequency=uint16_t(real_kla_frequency+adjustment);write(0xa4,uint8_t(real_kla_frequency));
    uint16_t loud=kladist<=8?8:kladist;
    if(old_kla_distance<64){
        if(kladist>=64){write(0x4c,63);voice_off(4,63);}
        else write(0x4c,uint8_t(loud));
    }else if(kladist<64){write(0x4c,uint8_t(loud));voice_on(4,uint16_t(klaqual)<<5);}
    old_kla_distance=kladist;
}
bool ambient_active(){return saucer_active||orb_on;}
void engine(bool audible,uint16_t currpmx,int16_t oil,int16_t speed,uint16_t engdam){
 // Retained only for callers outside cockpit_state. It must not be used by the
 // source engine block because that block deliberately leaves a wrapped AX.
 if(!audible)return;
 if(!currpmx){voice_off(0,0x40);return;}
 if(speed<0)return;
 uint32_t pitch=(uint32_t(uint16_t(oil))+0x780u+uint16_t(speed))/80u;
 if(pitch<0x40)pitch=0x40;
 if(pitch>0x400)pitch=0x400;
 voice_on(0,uint16_t(pitch));
 uint16_t ax=uint16_t(engdam<<1);
 if(int16_t(ax)<0)return;
 ax=uint16_t(ax-16);ax=uint16_t(-int16_t(ax));
 if(int16_t(ax)<0)ax=0;
 write(0x40,uint8_t(ax));
}
}
extern "C" {
void cc_audio_reset(){cc::audio::reset();}
void cc_audio_clock_advance(uint32_t ticks){cc::audio::advance(ticks);}
void cc_audio_write(uint32_t r,uint32_t v){cc::audio::write(uint8_t(r),uint8_t(v));}
void cc_audio_register_init(){cc::audio::register_init();}
void cc_audio_init_voices(){cc::audio::init_voices();}
void cc_audio_voice_on(uint32_t v,uint32_t ax){cc::audio::voice_on(uint8_t(v),uint16_t(ax));}
void cc_audio_voice_off(uint32_t v,uint32_t ax){cc::audio::voice_off(uint8_t(v),uint16_t(ax));}
void cc_audio_preset(uint32_t p){if(p<=uint32_t(cc::audio::Preset::kla))cc::audio::preset(cc::audio::Preset(p));}
void cc_audio_snare(uint32_t wind){cc::audio::snare_sound(wind!=0);}
void cc_audio_shot(uint32_t off_ax){cc::audio::shot(uint16_t(off_ax));}
void cc_audio_missile(uint32_t wind){cc::audio::missile(wind!=0);}
void cc_audio_crash(){cc::audio::crash();}
void cc_audio_crash_landing(){cc::audio::crash_landing();}
void cc_audio_stop_blam(){cc::audio::stop_blam();}
void cc_audio_scrape_stop(){cc::audio::scrape_stop();}
void cc_audio_reset_missile_patch(){cc::audio::reset_missile_patch();}
void cc_audio_tower(){cc::audio::tower();}
void cc_audio_collision_blam(){cc::audio::collision_blam();}
void cc_audio_explosion(uint32_t off_ax){cc::audio::explosion(uint16_t(off_ax));}
void cc_audio_big_boom(){cc::audio::big_boom();}
void cc_audio_stall(uint32_t active){cc::audio::stall(active!=0);}
void cc_audio_engine(uint32_t a,uint32_t c,int32_t oil,int32_t speed,uint32_t damage){cc::audio::engine(a!=0,uint16_t(c),int16_t(oil),int16_t(speed),uint16_t(damage));}
void cc_audio_sound_off(){cc::audio::sound_off();}
void cc_audio_sound_on(uint32_t ejected){cc::audio::sound_on(ejected!=0);}
void cc_audio_pause(){cc::audio::pause();}
void cc_audio_eject(){cc::audio::eject();}
void cc_audio_landing_screech(){cc::audio::landing_screech();}
void cc_audio_timer_tick(){cc::audio::timer_tick();}
void cc_audio_ambient_frame(uint32_t od,uint32_t of,uint32_t sd,uint32_t sf,uint32_t kd,uint32_t kq){cc::audio::ambient_frame(uint16_t(od),uint8_t(of),uint16_t(sd),uint8_t(sf),uint16_t(kd),uint8_t(kq));}
uint32_t cc_audio_ambient_active(){return cc::audio::ambient_active();}
uint32_t cc_audio_register_ptr(){return cc::audio::register_ptr();}
uint32_t cc_audio_editor_buffer(){return cc::audio::editor_buffer();}
void cc_audio_editor_reset(){cc::audio::editor_reset();}
uint32_t cc_audio_editor_commit(uint32_t index,uint32_t pairs){return cc::audio::editor_commit(uint8_t(index),uint8_t(pairs));}
uint32_t cc_audio_event_ptr(){return cc::audio::event_ptr();}
uint32_t cc_audio_event_capacity(){return 32768;}
uint32_t cc_audio_event_sequence(){return cc::audio::event_sequence();}
uint32_t cc_audio_raw_ticks(){return cc::audio::raw_ticks();}
uint32_t cc_audio_event_at(uint32_t sequence){return cc::audio::event_at(sequence);}
uint32_t cc_audio_event_tick_at(uint32_t sequence){return cc::audio::event_tick_at(sequence);}
}
