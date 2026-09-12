// Other Worlds MAP.ASM mbdrawscene/DD43 and mbguts/E0E3 continuations.
import {renderMapFrame,updateMapControls} from './map-renderer.mjs';
const check=(status,phase)=>{if(status)throw Error(`Original mission builder ${phase} returned ${status}`);};
export function renderBuilderScene(session,renderWorld,rawFrameTicks){
    const e=session.e,world=session.world,m=session.ds;
    if(!world)throw Error('Original mission builder requires the live world');
    const u=p=>m.getUint16(p,true),b=p=>m.getUint8(p),byte=(p,v)=>m.setUint8(p,v),word=(p,v)=>m.setUint16(p,v,true);
    const page=b(0xd5d)?1:0,nomove=!!e.cc_builder_nomove();
    e.cc_builder_scene_begin(rawFrameTicks);e.cc_video_load(page);
    if(!nomove){check(e.cc_epage_state_page(page),'page erase');e.cc_world_horizon();}
    const seedIn=()=>e.cc_raster_set_seed(m.getUint32(0xf722,true)),seedOut=()=>m.setUint32(0xf722,e.cc_raster_get_seed(),true);
    seedIn();
    const camera={position_cm:[0,4,8].map(i=>m.getInt32(0xb05+i,true)),angles_turn65536:[0,2,4].map(i=>u(0xb11+i))};
    const report=renderWorld(e,session.snapshot,camera,{views:true,liveWorld:world,groundPoints:false,preserveFrame:true,objectsOnly:true,
        renderObjects(draw){
            const drawMemory=(p,horizon=false)=>{
                if(!u(p+24))return;
                check(e.cc_distance_object(p)===2?2:0,'object distance');seedIn();draw(world.object(p),horizon);seedOut();
                const code=b(p+18);if(code<3)byte(0x205d,code===2&&m.getUint32(p+20,true)>m.getUint32(p+36,true)?4:0);
            };
            if(!nomove){byte(0x1ba9,255);drawMemory(0x1c6f,true);byte(0x1ba9,0);}
            // drawmemseg always runs, including getclosest's nomove scenes.
            const bytes=new Uint8Array(m.buffer,m.byteOffset,65536),cache=new Uint8Array(world.cache.buffer,world.cache.byteOffset,65536);
            for(let p=0;p<0xff88;p+=74)if(world.cache.getUint16(p+24,true)){
                bytes.set(cache.subarray(p,p+74),0x2159);
                if(!b(0xf4e))throw Error('Unreachable original debug drawmemflag=0 state');
                drawMemory(0x2159);
                check(e.cc_promote_cached(p),'far activation');
            }
            check(e.cc_move_effects(),'object movement');
            e.cc_builder_near_begin();
            for(;;){const p=e.cc_builder_near_next();if(!p)break;if(p===0xffffffff)throw Error('Original builder near table is invalid');drawMemory(p);}
            check(e.cc_builder_scene_finish(),'tile update');seedIn();
        }
    });
    seedOut();e.cc_video_store(page);session.page=e.cc_camera_flip_page();return report;
}
export function advanceBuilder(session,renderWorld,input={},ticks=session.rawFrameTicks){
    const e=session.e;check(e.cc_builder_begin_frame(),'frame prefix');
    session.report=renderBuilderScene(session,renderWorld,ticks);e.cc_builder_after_frame();e.cc_builder_numbers();
    let result=e.cc_builder_dispatch();
    for(let stage=0;stage<16;stage++){
        if(result===1){session.report=renderBuilderScene(session,renderWorld,0);result=e.cc_builder_dispatch();}
        else if(result===4){let x=input.x??0,y=input.y??0;if(input.events)({x,y}=session.axes(input));e.cc_builder_axes(x,y);result=e.cc_builder_dispatch();}
        else{check(result===3?3:0,'controls');session.page=session.ds.getUint8(0xd5d)?1:0;return result===2;}
    }
    throw Error('Original builder continuation did not settle');
}
// Returns true only when map itself has ended. The B-release call returns to
// the same mapkey continuation after M/Esc exits the builder.
export function updateOriginalMap(session,renderWorld,input={}){
    const e=session.e;
    if(!e.cc_edition_is_other_worlds()){
        session.report=renderMapFrame(session,renderWorld,session.rawFrameTicks);return updateMapControls(session,input);
    }
    if(e.cc_builder_active()){
        if(!advanceBuilder(session,renderWorld,input))return false;
        return updateMapControls(session,input);
    }
    session.report=renderMapFrame(session,renderWorld,session.rawFrameTicks);e.cc_builder_map_numbers();
    if(session.ds.getUint8(0x26c)===0xb0){
        e.cc_builder_begin();if(!advanceBuilder(session,renderWorld,input,0))return false;
    }
    return updateMapControls(session,input);
}
