import {DemoStream} from './demo-stream.mjs';
import {renderWorld,worldRGBA} from './world.mjs';
import {renderRemoteInset} from './remote-inset.mjs';
import {updateOriginalMap} from './builder-renderer.mjs';
import {LiveWorld} from './live-world.mjs';
// Original PIT divisor and a fixed normal frame value. Native comparison traces
// use the same explicit tick input; rendering speed never rescales the physics.
export function dosClock(){const d=new Date();return Math.floor(d.getMilliseconds()/10)|(d.getSeconds()<<8)|(d.getMinutes()<<16)|(d.getHours()<<24);}
export const PIT_HZ=1193182/1024,FRAME_TICKS=5,FRAME_MS=FRAME_TICKS*8/PIT_HZ*1000;
const SCANS={Escape:1,Digit1:2,Digit2:3,Digit3:4,Digit4:5,Digit5:6,Digit6:7,Digit7:8,Digit8:9,Digit9:10,Digit0:11,Minus:12,Equal:13,Backspace:14,Tab:15,KeyQ:16,KeyW:17,KeyE:18,KeyR:19,KeyT:20,KeyY:21,KeyU:22,KeyI:23,KeyO:24,KeyP:25,BracketLeft:26,BracketRight:27,Enter:28,ControlLeft:29,ControlRight:29,KeyA:30,KeyS:31,KeyD:32,KeyF:33,KeyG:34,KeyH:35,KeyJ:36,KeyK:37,KeyL:38,Semicolon:39,Quote:40,Backquote:41,ShiftLeft:42,Backslash:43,KeyZ:44,KeyX:45,KeyC:46,KeyV:47,KeyB:48,KeyN:49,KeyM:50,Comma:51,Period:52,Slash:53,ShiftRight:54,NumpadMultiply:55,AltLeft:56,Space:57,CapsLock:58,F1:59,F2:60,F3:61,F4:62,F5:63,F6:64,F7:65,F8:66,F9:67,F10:68,NumLock:69,ScrollLock:70,Home:71,ArrowUp:72,PageUp:73,NumpadSubtract:74,ArrowLeft:75,Numpad5:76,ArrowRight:77,NumpadAdd:78,End:79,ArrowDown:80,PageDown:81,Insert:82,Delete:83};
export class FlightInput {
    constructor(tick=0){this.reset(tick);}
    reset(tick){this.keys=new Map();this.initialTick=tick&65535;this.scan=null;this.events=[];this.lastConsume=tick&65535;}
    alias(code){return ({Numpad4:'ArrowLeft',Numpad6:'ArrowRight',Numpad8:'ArrowUp',Numpad2:'ArrowDown',Numpad7:'Home',Numpad9:'PageUp',Numpad1:'End',Numpad3:'PageDown',Numpad0:'Insert',NumpadDecimal:'Delete',NumpadEnter:'Enter'})[code]??code;}
    record(code){let s=this.keys.get(code);if(!s){s={down:false,since:0,held:0,last:this.initialTick};this.keys.set(code,s);}return s;}
    key(code,down,tick){
        code=this.alias(code);tick&=65535;this.scan={code,down};if(SCANS[code]!==undefined)this.events.push({scan:SCANS[code]|(down?0:128),tick});const s=this.record(code);
        if(s.down===down)return;
        if(s.down)s.held=(s.held+tick-s.since)&65535;
        s.down=down;s.since=tick;
    }
    pressure(code,tick){
        tick&=65535;const s=this.record(this.alias(code)),elapsed=(tick-s.last)&65535;
        const held=(s.held+(s.down?tick-s.since:0))&65535;s.held=0;s.last=tick;if(s.down)s.since=tick;
        return !elapsed||elapsed>=32768?0:Math.min(512,Math.floor(held*512/elapsed));
    }
    consume(tick,desiredView=false){
        const p=code=>this.pressure(code,tick),held=code=>Boolean(this.keys.get(code)?.down);
        let x=0,y=0;if(!desiredView){const left=p('ArrowLeft'),right=p('ArrowRight'),up=p('ArrowUp'),down=p('ArrowDown');x=right-left;y=down-up;}
        const flaps=this.scan?.code==='KeyF'&&!this.scan.down;if(flaps)this.scan=null;
        const elapsed=(tick-this.lastConsume)&65535,events=this.events.splice(0).map(event=>({scan:event.scan,fraction:elapsed?Math.min(1,((event.tick-this.lastConsume)&65535)/elapsed):1}));this.lastConsume=tick&65535;
        return {events,x,y,rudder:p('KeyZ')-p('KeyX')+p('Home')-p('PageUp'),
            brake:held('Period')||held('Insert'),plus:held('Equal')||held('NumpadAdd'),minus:held('Minus')||held('NumpadSubtract'),flaps,
            weapons:Number(held('Space'))|Number(held('ShiftLeft'))*2|Number(held('KeyC'))*4,walkForward:held('Numpad5'),walkBackward:held('Insert')};
    }
}
export class FlightSession {
    constructor(exports,snapshot,initial,cockpit,tiles,{fresh=false,helpImages=new Map(),demoPlayback=null}={}){
        this.helpImages=helpImages;this.demo=exports.cc_edition_is_other_worlds?.()?new DemoStream(exports,demoPlayback):null;
        this.fresh=fresh;
        this.e=exports;this.originalSnapshot=snapshot;this.initial=new Uint8Array(initial).slice();this.cockpit=cockpit;this.tiles=tiles;
        this.reset();
    }
    reset(){
        const e=this.e;this.snapshot=structuredClone(this.originalSnapshot);this.frame=0;this.page=0;this.framePhase="new";this.paused=false;this.map=false;this.status=0;this.exit=null;this.colorWait=false;this.stars=false;this.starsKeys=[];this.uiRequest=null;
        this.rawFrameTicks=FRAME_TICKS*8;e.cc_modal_child_reset();this.modalWorld=null;
        this.ds=new DataView(e.memory.buffer,e.cc_flight_state(),65536);new Uint8Array(e.memory.buffer,e.cc_flight_state(),65536).set(this.initial.subarray(0,65536));
        this.runtime=new Uint32Array(e.memory.buffer,e.cc_runtime_state(),3);this.runtime.set([this.initial.length>=131072?new DataView(this.initial.buffer,this.initial.byteOffset).getUint16(65536+0x43,true):0,0,this.initial.length>=131072?new DataView(this.initial.buffer,this.initial.byteOffset).getUint16(65536+0x37,true):0]);
        if(this.tiles){this.world=new LiveWorld(e,this.snapshot,this.initial,this.tiles);this.world.reset();}
        new Uint8Array(e.memory.buffer,e.cc_weapon_cockpit_image(),640*350).set(this.cockpit);e.cc_weapon_cockpit_ready(1);
        this.demo?.reset();
        this.input={};this.paletteSerial=new Uint32Array(e.memory.buffer,e.cc_enemy_palette_event(),1)[0];e.cc_lifecycle_reset();e.cc_sortie_reset();e.cc_high_altitude_reset();e.cc_color_input_reset();e.cc_pause_reset();e.cc_map_reset();e.cc_builder_reset();e.cc_sortie_timer_reset(dosClock());e.cc_audio_reset();e.cc_audio_register_init();e.cc_audio_init_voices();e.cc_radio_reset();e.cc_connect_lifecycle_world();this.releaseKeys();
        if(!this.fresh)e.cc_flight_refresh();e.cc_instrument_reset();new Uint8Array(e.memory.buffer,e.cc_video_memory(),65536*8).fill(0);
        for(let page=0;page<2;page++){
            new Uint8Array(e.memory.buffer,e.cc_framebuffer(),640*350).set(this.cockpit);e.cc_video_store(page);this.instrumentValues();e.cc_instrument_draw_page(page);
        }
        e.cc_hud_damage_init();e.cc_hud_redraw_buttons();if(this.fresh)e.cc_video_load(this.page);else{this.draw();e.cc_camera_present();}
    }
    get frameMilliseconds(){return (this.framePhase==='new'?(this.demo?.rawTicks(FRAME_TICKS*8)??FRAME_TICKS*8):this.rawFrameTicks)/PIT_HZ*1000;}
    releaseKeys(){
        const bytes=new Uint8Array(this.e.memory.buffer,this.e.cc_flight_state(),65536);bytes.fill(0,0x2c7,0x6d7);bytes[0x26c]=bytes[0x26d]=0;
        for(let i=0;i<128;i++)this.ds.setUint16(0x2d7+i*8+6,this.runtime[0],true);
    }
    drainLifecycle(){
        const e=this.e,events=new Uint32Array(e.memory.buffer,e.cc_lifecycle_events(),1+64*22);
        for(let i=0;i<events[0];i++){
            const kind=events[1+i*22];
            if(kind===1)throw Error('Original wreck persistence hook did not complete');
            if(kind===3){
                const saved=new Uint8Array(e.memory.buffer,e.cc_framebuffer(),640*350).slice();
                for(let page=0;page<2;page++){new Uint8Array(e.memory.buffer,e.cc_framebuffer(),640*350).set(this.cockpit);e.cc_video_store(page);}
                e.cc_hud_damage_init();e.cc_hud_redraw_buttons();new Uint8Array(e.memory.buffer,e.cc_framebuffer(),640*350).set(saved);
            }
        }
        e.cc_lifecycle_clear_events();
    }
    u(at){return this.ds.getUint16(at,true);}
    s(at){return this.ds.getInt16(at,true);}
    camera(){return {position_cm:[0,4,8].map(n=>this.ds.getInt32(0xb05+n,true)),angles_turn65536:[0,2,4].map(n=>this.u(0xb11+n))};}
    viewCamera(){const camera=this.camera();if(this.ds.getUint8(0xf3b))camera.angles_turn65536=[0,2,4].map(n=>this.u(0xf3d+n));return camera;}
    updatePalette(){
        const e=this.e,event=new Uint32Array(e.memory.buffer,e.cc_enemy_palette_event(),65),palette=this.snapshot.palette;
        const attribute=palette.base.map(v=>v.attribute_dac_register);if(palette.attribute_index8_override)attribute[8]=palette.attribute_index8_override.dac_register;
        const apply=(index,rgb)=>{for(let color=0;color<16;color++)if(attribute[color]===index)palette.render_rgb8[color]=rgb.map(v=>((v&63)<<2)|((v&63)>>4));};
        if(event[0]!==this.paletteSerial){this.paletteSerial=event[0];for(let i=1;i<65;i+=4)apply(event[i],[event[i+1],event[i+2],event[i+3]]);}
        const fade=e.cc_camera_palette();if(fade<256)for(let i=0;i<16;i++){const p=0x1e6e+i*4;apply(this.ds.getUint8(p),[1,2,3].map(k=>(this.ds.getUint8(p+k)*fade)>>8));}
    }
    instrumentValues(){
        this.e.cc_instrument_oil_color(this.ds.getUint8(0xf1fc));
        const values=[this.ds.getUint32(0xb0d,true),this.u(0xf5c),...this.camera().angles_turn65536,this.u(0x1d9c),this.u(0x1d9e),this.u(0x1da0),this.u(0x1dc6),this.u(0x1de2),this.u(0x1de4),this.u(0x1e5a),this.u(0x1e5c),this.u(0x1de),this.frame];
        new Uint32Array(this.e.memory.buffer,this.e.cc_instrument_input(),15).set(values);
    }
    draw(advanceWorld=false){
        const e=this.e;this.ds.setUint16(0x1b90,this.page?0:0x7e00,true);this.ds.setUint8(0xd5d,this.page?255:0);e.cc_video_load(this.page);e.cc_epage_state_page(this.page);e.cc_cockpit_home_page(this.page);
        if(this.u(0x1c3e)!==0x1c26)e.cc_instrument_erase_page(this.page,this.frame);
        if(e.cc_camera_prepare())throw Error('Original camera preparation failed');
        e.cc_raster_set_seed(this.ds.getUint32(0xf722,true));
        this.report=renderWorld(e,this.snapshot,this.viewCamera(),{preserveFrame:true,advanceGround:advanceWorld,liveWorld:this.world,views:true,renderObjects:advanceWorld&&this.world?draw=>this.world.renderFrame(draw):undefined,
            afterObjects:draw=>{if(this.world)renderRemoteInset(e,this.world,draw);if(advanceWorld&&e.cc_camera_after_world())throw Error('Original camera target update failed');this.updatePalette();if(this.world&&e.cc_cockpit_prop_visible())draw(this.world.object(0x5bfa));}});
        this.ds.setUint32(0xf722,e.cc_raster_get_seed(),true);
        e.cc_video_store(this.page);
        if(e.cc_cockpit_gunsight_visible(Number(Boolean(this.input.walkForward)),Number(Boolean(this.input.walkBackward))))e.cc_cockpit_gunsight_page(this.page);
        if(this.u(0x1c3e)!==0x1c26){e.cc_cockpit_controls_page(this.page);this.instrumentValues();e.cc_instrument_draw_page(this.page);}this.page=e.cc_camera_flip_page();
    }
    drawNear(){
        const e=this.e,page=this.u(0x1b90)===0?1:0;
        e.cc_video_load(page);e.cc_raster_set_seed(this.ds.getUint32(0xf722,true));
        const report=renderWorld(e,this.snapshot,this.viewCamera(),{preserveFrame:true,objectsOnly:true,views:true,liveWorld:this.world,renderObjects:draw=>this.world.renderFrame(draw,{nearOnly:true})});
        this.ds.setUint32(0xf722,e.cc_raster_get_seed(),true);e.cc_video_store(page);return report;
    }
    finishSortie(direct=false){
        const e=this.e,bytes=new Uint8Array(e.memory.buffer,e.cc_flight_state(),65536);
        let phase=direct?e.cc_sortie_begin_exit(this.rawFrameTicks):e.cc_sortie_frame_end(this.rawFrameTicks);
        if(!phase)return 0;
        const result=this.exit??={};
        if(phase===1){
            this.drawNear();phase=e.cc_sortie_after_near(this.rawFrameTicks);
        }
        if(phase===2){e.cc_sortie_config_hash();result.config=bytes.slice(0xaf7,0xaf7+this.u(0xb2c));e.cc_sortie_config_unhash();phase=e.cc_sortie_after_config();}
        if(phase===3){result.raw=bytes.slice(0x1e4,0x1e4+140);phase=e.cc_sortie_after_results();}
        if(phase===4){
            if(e.cc_flush_tiles())throw Error('Original final world flush failed');
            if(e.cc_world_write_file_mode(0,e.cc_edition_is_other_worlds()))throw Error('Original final world serialization failed');
            const io=new Uint32Array(e.memory.buffer,e.cc_world_io_result(),4);result.world=new Uint8Array(e.memory.buffer,e.cc_world_file_buffer(),io[1]).slice();phase=e.cc_sortie_after_world();
        }
        result.raw??=bytes.slice(0x1e4,0x1e4+140);result.engineError=this.runtime[1];
        if(phase===5){e.cc_audio_sound_off();this.status=4;}return this.status;
    }
    enterMenu(kind){
        if(this.uiRequest)return true;
        const e=this.e;
        if(!e.cc_modal_tower_enter(dosClock())){e.cc_modal_tower_finish();return false;}
        // Original writeresfile precedes runtower. Supply this exact record to
        // the child UI/file adapter; it is not a completed sortie/career commit.
        this.modalResults=new Uint8Array(e.memory.buffer,e.cc_flight_state()+0x1e4,140).slice();
        e.cc_modal_tower_audio();
        let phase=e.cc_modal_child_begin();
        while(phase>=2&&phase<=6){
            // F6's original preparation is ordered and completes even when a
            // callee records cecode. API failures still identify a host boundary.
            if(phase===2&&e.cc_flush_tiles())throw Error('Original theater-info world flush failed');
            if(phase===3)this.drawNear();
            if(phase===4)e.cc_world_clean_objects();
            if(phase===5)e.cc_clear_bullets();
            if(phase===6){
                if(e.cc_world_write_file_mode(0,1))throw Error('Original theater-info world write failed');
                const io=new Uint32Array(e.memory.buffer,e.cc_world_io_result(),4);this.modalWorld=new Uint8Array(e.memory.buffer,e.cc_world_file_buffer(),io[1]).slice();
            }
            phase=e.cc_modal_child_advance();
        }
        if(phase===1)kind='calibration';
        else if(phase===8)kind='score-unavailable';
        else if(phase===9)kind='theater-unavailable';
        else if(e.cc_modal_child_kind()===1)kind='inflight-score';
        else if(e.cc_modal_child_kind()===2)kind='theater-info';
        this.uiRequest=kind;return true;
    }
    leaveMenu(){
        if(!this.uiRequest)return;
        const e=this.e;let phase=e.cc_modal_child_stage();
        if(phase===1)phase=e.cc_modal_child_calibration_result(this.runtime[1]);
        else if(phase===7)phase=e.cc_modal_child_exec_result(0,0);
        if(phase>=8&&phase<=10)e.cc_modal_child_advance();
        e.cc_modal_tower_return(dosClock());e.cc_modal_tower_finish();
        const event=new Uint32Array(e.memory.buffer,e.cc_modal_tower_palette(),66),palette=this.snapshot.palette;
        palette.attribute_index8_override={...(palette.attribute_index8_override??{}),dac_register:event[1]};
        const attributes=palette.base.map(v=>v.attribute_dac_register);attributes[8]=event[1];
        for(let i=2;i<66;i+=4)for(let c=0;c<16;c++)if(attributes[c]===event[i])palette.render_rgb8[c]=[event[i+1],event[i+2],event[i+3]].map(v=>((v&63)<<2)|((v&63)>>4));
        this.uiRequest=null;if(this.framePhase==='tower')this.framePhase='pause';
        e.cc_camera_present();
    }
    shortcut(coordinates){
        if(this.ds.getUint8(0xafc))return false;
        for(let i=0;i<3;i++)this.ds.setInt32(0x285+i*4,coordinates[i],true);
        this.ds.setInt16(0x291,coordinates[3],true);this.ds.setUint8(0xe56,255);return true;
    }
    beginSpace(){
        const e=this.e;let phase=e.cc_high_altitude_begin();if(!phase)return false;
        if(phase===1){if(e.cc_flush_tiles())throw Error('Original space transition flush failed');phase=e.cc_high_altitude_after_flush();}
        if(phase===2){this.drawNear();phase=e.cc_high_altitude_after_near();}
        if(phase===3){e.cc_instrument_erase_page(this.page,this.frame);this.page=e.cc_camera_flip_page();e.cc_instrument_erase_page(this.page,this.frame);phase=e.cc_high_altitude_after_presentation(dosClock());}
        if(phase===4){
            new Uint8Array(e.memory.buffer,e.cc_stars_video(),524288).set(new Uint8Array(e.memory.buffer,e.cc_video_memory(),524288));
            const color=this.snapshot.palette.render_rgb8[11].map(v=>v>>2),index=this.snapshot.palette.base[11].attribute_dac_register;
            this.starsTick=e.cc_legacy_ticks();e.cc_stars_begin(index,...color,this.starsTick);this.stars=true;this.starsKeys=[];
        }
        return true;
    }
    spaceStep(input){
        const e=this.e;for(const event of input.events??[])if(event.scan===28||event.scan===1)this.starsKeys.push(event.scan===28?13:27);
        e.cc_clock_advance(FRAME_TICKS*8);const tick=e.cc_legacy_ticks();if(tick===this.starsTick)return 0;this.starsTick=tick;
        const result=e.cc_stars_frame(this.starsKeys.shift()??0,tick);if(result===3)throw Error('Original space animation arithmetic failed');
        const palette=new Uint32Array(e.memory.buffer,e.cc_stars_palette(),4),rgb=[...palette.slice(1)].map(v=>(v<<2)|(v>>4));
        for(let i=0;i<16;i++)if((i===8?63:this.snapshot.palette.base[i].attribute_dac_register)===palette[0])this.snapshot.palette.render_rgb8[i]=rgb;
        e.cc_stars_present();
        if(result){this.stars=false;new Uint8Array(e.memory.buffer,e.cc_video_memory(),524288).set(new Uint8Array(e.memory.buffer,e.cc_stars_video(),524288));e.cc_high_altitude_after_stars(dosClock(),0,FRAME_TICKS*8);return this.finishSortie();}
        return 0;
    }
    axes(input={}){
        if(!input.events)return {x:input.x??0,y:input.y??0};
        if(this.ds.getUint8(0xaf6)){
            if(!input.joystick?.connected)throw Error('Joystick disconnected.');
            const packed=this.e.cc_joystick_input(input.joystick.rawX,input.joystick.rawY)>>>0;
            if(packed===0xffffffff)throw Error('Original joystick calibration has a zero span.');
            return {x:(packed&65535)-512,y:(packed>>>16)-512};
        }
        const packed=this.e.cc_lifecycle_arrows();return {x:(packed<<16)>>16,y:packed>>16};
    }
    step(input={}){
        if(this.status)return this.status;if(this.uiRequest)return 0;
        const e=this.e,bytes=new Uint8Array(e.memory.buffer,e.cc_flight_state(),65536),held=scan=>e.cc_lifecycle_key_down(scan);
        const released=scan=>{if(bytes[0x26c]!==(scan|128))return false;bytes[0x26c]=0;return true;};
        try{
            if(this.stars)return this.spaceStep(input);
            this.input=input;if(this.framePhase==='new')this.rawFrameTicks=this.demo?.rawTicks(FRAME_TICKS*8)??FRAME_TICKS*8;const before=this.runtime[0];e.cc_clock_advance(this.rawFrameTicks);
            if(input.events)for(const event of input.events)e.cc_lifecycle_key_event(event.scan,(before+Math.round(event.fraction*this.rawFrameTicks))&65535);
            if(this.colorWait)return this.continueFlight(input,true);
            if(this.framePhase==='new'){e.cc_frame_input_prefix(this.rawFrameTicks);this.framePhase='radio';}
            if(this.framePhase==='radio'){
                const radioState=new Uint32Array(e.memory.buffer,e.cc_radio_state(),6);let radio;
                if(radioState[0])radio=e.cc_radio_update();
                else{radio=e.cc_modal_radio_draw_queued(dosClock());if(radio===1)e.cc_modal_radio_wait_start(dosClock());}
                if(radio===2)throw Error('Original radio record is invalid');if(radio){this.radio=true;e.cc_video_load(radioState[4]);return 0;}this.radio=false;this.framePhase='tower';
            }
            if(this.framePhase==='tower'){
                if(!bytes[0xeaf]&&(bytes[0xf4d]||bytes[0x1ca])){if(this.enterMenu(bytes[0x1baa]?'calibration':bytes[0xf4d]===0x7f?'score-unavailable':bytes[0xf4d]===0x7d?'theater-unavailable':bytes[0xf4d]?'boss':'tower'))return 0;}
                this.framePhase='pause';
            }
            if(this.framePhase==='pause'){
                let stage=e.cc_pause_stage()?e.cc_pause_update(dosClock()):e.cc_pause_begin(dosClock());
                if(stage===2){const image=this.helpImages.get(e.cc_pause_image_id());if(image)new Uint8Array(e.memory.buffer,e.cc_pause_image_buffer(),224000).set(image);stage=e.cc_pause_after_image(Number(!image),dosClock());}
                this.paused=!!stage;if(stage){e.cc_camera_present();return 0;}e.cc_frame_after_pause();this.framePhase='map';
            }
            if(this.framePhase==='map'){
                const requested=e.cc_map_request();if(requested===2)throw Error('Original map initialization failed');
                if(e.cc_map_active()){this.map=true;if(!updateOriginalMap(this,renderWorld,input)){e.cc_camera_present();return 0;}this.map=false;}
            }
            this.framePhase='render';
            this.frame++;if(this.frame>=16384)this.frame=128;this.ds.setUint16(0x1bc8,this.frame,true);this.draw(true);
            this.status=e.cc_cockpit_engine_update();if(this.status)return this.status;
            let x=input.x??0,y=input.y??0,rudder=input.rudder??0,brake=Number(Boolean(input.brake));
            if(input.events){({x,y}=this.axes(input));rudder=e.cc_lifecycle_scalekey(44,512)-e.cc_lifecycle_scalekey(45,512)+e.cc_lifecycle_scalekey(71,512)-e.cc_lifecycle_scalekey(73,512);brake=e.cc_joystick_brake(held(52)|held(82)*2|held(42)*4);}
            this.status=e.cc_flight_step(x,y,rudder,brake,this.u(0x1e5e));this.demo?.capture();if(this.status)return this.status;
            e.cc_frame_geometry();
            if(e.cc_sortie_dvel_average())throw Error('Original acceleration filter failed');this.applyFade(e.cc_sortie_palette());
            if(this.beginSpace())return 0;
            return this.continueFlight(input);
        }catch(error){this.failure=error.message;this.status=5;return this.status;}
    }
    applyFade(fade){
        if(fade>=256)return;const event=new Uint32Array(this.e.memory.buffer,this.e.cc_enemy_palette_event(),65);
        const palette=this.snapshot.palette,attribute=palette.base.map(v=>v.attribute_dac_register);if(palette.attribute_index8_override)attribute[8]=palette.attribute_index8_override.dac_register;
        for(let i=0;i<16;i++){const p=0x1e6e+i*4,index=this.ds.getUint8(p),rgb=[1,2,3].map(k=>{const v=this.ds.getUint8(p+k)*fade>>8;return (v<<2)|(v>>4);});for(let c=0;c<16;c++)if(attribute[c]===index)palette.render_rgb8[c]=rgb;}
    }
    continueFlight(input,resuming=false){
        const e=this.e,bytes=new Uint8Array(e.memory.buffer,e.cc_flight_state(),65536),held=scan=>e.cc_lifecycle_key_down(scan);
        const released=scan=>{if(bytes[0x26c]!==(scan|128))return false;bytes[0x26c]=0;return true;};
        try{
            const color=e.cc_color_input();if(color===3)throw Error('Original color arithmetic failed');this.colorWait=!!color;
            if(e.cc_color_input_palette_dirty()){
                const palette=this.snapshot.palette,attribute=palette.base.map(v=>v.attribute_dac_register);if(palette.attribute_index8_override)attribute[8]=palette.attribute_index8_override.dac_register;
                for(let i=0;i<16;i++){const p=0x1eae+i*4,index=bytes[p],rgb=[1,2,3].map(k=>(bytes[p+k]<<2)|(bytes[p+k]>>4));for(let c=0;c<16;c++)if(attribute[c]===index)palette.render_rgb8[c]=rgb;}
            }
            if(color)return 0;
            e.cc_aircraft_try_reenter();e.cc_enemy_rescue_request();e.cc_view_input();e.cc_cockpit_prop_update();
            const weapons=input.events?(held(57)|held(42)*2|held(46)*4):(input.weapons??0);
            e.cc_weapon_configure(weapons);e.cc_hud_altitude_warning();this.status=bytes[0xaf6]?e.cc_joystick_weapons(weapons,input.joystick?.port??48):e.cc_weapon_input(weapons);if(this.status)return this.status;
            this.status=e.cc_weapon_late_input(e.cc_legacy_ticks());if(this.status)return this.status;
            e.cc_view_remote_input();
            e.cc_demo_frame_controls(input.events?(held(13)|held(78)):Number(Boolean(input.plus)),input.events?held(76):Number(Boolean(input.walkForward)),input.events?held(82):Number(Boolean(input.walkBackward)));
            e.cc_flight_throttle(input.events?(held(13)|held(78)):Number(Boolean(input.plus)),input.events?(held(12)|held(74)):Number(Boolean(input.minus)));
            if(e.cc_aircraft_resolve_crash()){this.drainLifecycle();return this.finishSortie(true);}
            e.cc_ground_input();
            e.cc_pilot_walk(input.events?held(76):Number(Boolean(input.walkForward)),input.events?held(82):Number(Boolean(input.walkBackward)));
            let flap;if(this.demo){if(!input.events&&input.flaps)bytes[0x26c]=0xa1;flap=e.cc_demo_flap_input();}else if(input.events?released(33):input.flaps){e.cc_flight_flaps();flap=1;}
            if(flap)for(const packed of bytes[0xf59]?[0x0f07,0x0600]:[0x070f,0x0006])e.cc_hud_button(2,packed);
            e.cc_hud_complete_button();
            e.cc_camera_rear_input();
            if(released(30)){bytes[0xe59]^=255;for(const packed of bytes[0xe59]?[0x0f07,0x0100]:[0x070f,0x0001])e.cc_hud_button(0,packed);}
            if(this.demo){if(!input.events&&input.eject)bytes[0x26c]=0x92;e.cc_demo_eject_input();if(!input.events&&input.chute)bytes[0x26c]=0xb9;e.cc_demo_chute_input();}
            else{if(input.events?released(18):input.eject)e.cc_aircraft_eject();if(input.events?released(57):input.chute)e.cc_aircraft_pull_chute();}
            if(this.finishSortie())return this.status;this.framePhase="new";
            this.drainLifecycle();if(this.runtime[1])throw Error(`Original engine diagnostic ${this.runtime[1]}`);
            e.cc_camera_present();return this.status;
        }catch(error){this.failure=error.message;this.status=5;return this.status;}
    }
    rgba(){return worldRGBA(this.e,this.snapshot);}
}
