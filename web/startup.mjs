import {applyPackagedDemoFlags} from './packaged-demos.mjs';
import {FlightSession} from './session.mjs';
import {decodeName} from './characters.mjs';
// The original reads the BIOS tick counter when seeding its RNG. This browser
// adapter supplies the equivalent time-of-day counter as an explicit input.
export function biosTick(){const d=new Date();return Math.floor((d.getHours()*3600000+d.getMinutes()*60000+d.getSeconds()*1000+d.getMilliseconds())*1193182/(65536*1000))>>>0;}
export function initializeFlight(e,assets,career,{seed=biosTick(),extraPlane=false,memSegment=0x2263,controlFlags=8,joystickStartup=null,startAirfield=null,demoPlayback=null,packagedDemo=null,editorWorld=null}={}){
    if(career?.importing)throw Error('Wait for the save import to finish before flying.');
    if(e.cc_edition_set(assets.editionId??0))throw Error('Unsupported original edition profile.');
    const state=e.cc_flight_state(),ds=new Uint8Array(e.memory.buffer,state,65536),dv=new DataView(ds.buffer,ds.byteOffset,ds.length),u=p=>dv.getUint16(p,true);
    const put=bytes=>{const ptr=e.cc_world_file_buffer();new Uint8Array(e.memory.buffer,ptr,bytes.length).set(bytes);return ptr;};
    e.cc_startup_seed(state,put(assets.initialDS),0x12ca);new Uint32Array(e.memory.buffer,e.cc_runtime_state(),3).fill(0);
    e.cc_world_clean_objects();e.cc_startup_random_seed(state,seed);
    const theater=editorWorld?{world:editorWorld}:packagedDemo??career.theater();
    if(editorWorld){if(!e.cc_edition_is_other_worlds())throw Error('The mission builder requires Other Worlds.');e.cc_builder_startup_options(controlFlags);extraPlane=false;}
    else if(packagedDemo){if(!e.cc_edition_is_other_worlds())throw Error('Packaged demos require Other Worlds.');applyPackagedDemoFlags(ds);extraPlane=false;}
    else{const pilot=career.current(),p=put(pilot);e.cc_options_context(p+0x234,Number(!!theater));e.cc_startup_pilot_options(state,p+0x234,controlFlags,pilot[0x25e],Number(!!theater),Number(extraPlane));}
    e.cc_world_allocate(memSegment);if(assets.config?.length===52)e.cc_startup_load_cfg(state,put(assets.config));
    const finishInitialization=()=>{
    e.cc_startup_before_world(state);e.cc_startup_ground(state,put(assets.groundTables));e.cc_startup_matrices(state);
    const stem=theater?.header?decodeName(theater.header.subarray(28,38)).toLowerCase():'deftower',twr=packagedDemo?null:assets.towers?.get(stem);
    if(twr)ds.set(twr.subarray(0,80),0x1a1e);
    if(twr?.[0]===88){
        const first=new TextDecoder().decode(twr).split(/\r?\n/)[0],numbers=first.slice(1).trim().split(/\s+/).map(Number);
        if(numbers.length>=6&&numbers.slice(0,6).every(Number.isInteger)){const p=e.cc_browser_workspace(),params=new Int32Array(e.memory.buffer,p,7);params.fill(0);params.set(numbers.slice(0,6));const validLast=Number.isInteger(numbers[6]);if(validLast)params[6]=numbers[6];e.cc_startup_theater_params_mode(state,p,Number(validLast),e.cc_edition_is_other_worlds());}
    }
    if(theater){const p=put(theater.world);if(!editorWorld&&startAirfield!==null&&e.cc_towers_start(p,theater.world.length,startAirfield))throw Error('Invalid starting airfield.');if(e.cc_world_read_file_mode(theater.world.length,0,e.cc_edition_is_other_worlds()))throw Error('Could not load the original theater world.');}
    if(e.cc_startup_after_world(state)===-1){if(e.cc_initperms())throw Error('Could not generate the original world.');e.cc_startup_generated_observer(state);e.cc_startup_recalcmats(state);}
    e.cc_startup_palette(state,put(assets.modePalette));e.cc_startup_sky_detail(state);if(e.cc_world_update_tiles())throw Error('Could not load the original starting region.');
    const snapshot=structuredClone(assets.snapshot),ground=new DataView(e.memory.buffer,e.cc_startup_ground_buffer(),4566),points=base=>Array.from({length:ground.getUint16(0,true)},(_,i)=>{const p=base+i*14;return {segment_offset:p,position_cm:[0,4,8].map(k=>ground.getInt32(p+k,true)),fixed_flag:ground.getUint16(p+12,true)};});
    Object.assign(snapshot.special.ground_points,{ground:points(2),stars:points(0x8fa),ground_enabled:!!ds[0x1bac],stars_enabled:!!ds[0x1bad],color_index:ds[0x1bae]});
    snapshot.palette.base=Array.from({length:16},(_,i)=>({index:i,attribute_dac_register:ds[0x1e6e+i*4],dac6:[1,2,3].map(k=>ds[0x1e6e+i*4+k])}));
    snapshot.palette.render_rgb8=snapshot.palette.base.map((_,i)=>{const source=i===8?15:i;return snapshot.palette.base[source].dac6.map(v=>{const faded=v*255>>8;return(faded<<2)|(faded>>4);});});
    Object.assign(snapshot.special.sun,{sunangle:u(0x280),sun_color_index:ds[0x537d],diameters:[u(0x5386),u(0x5390)]});
    if(demoPlayback&&e.cc_edition_is_other_worlds()){ds[0xaeb]=255;ds[0x1e3b]=255;}
    const initial=new Uint8Array(196608),world=new Uint8Array(e.memory.buffer,e.cc_universe_memory(),0x45f90);initial.set(ds);initial.set(world.subarray(0,65536),131072);
    return {snapshot,initial,tiles:world.slice(0xff90),extraPlane};
    };
    // Original calibration is after CFG load and before world/observer startup.
    // Existing synchronous callers remain synchronous when no hook is supplied.
    if(joystickStartup&&ds[0xaf6])return Promise.resolve(joystickStartup({state,ds})).then(finishInitialization);
    return finishInitialization();
}
export function createFlight(e,assets,career,options={}){
    const make=boot=>{const session=new FlightSession(e,boot.snapshot,boot.initial,assets.cockpit,boot.tiles,{fresh:true,helpImages:assets.helpImages,demoPlayback:options.demoPlayback??null});session.requestedExtraPlane=Number(boot.extraPlane);session.packagedDemo=options.packagedDemo??null;return session;};
    const boot=initializeFlight(e,assets,career,options);return boot?.then?boot.then(make):make(boot);
}
