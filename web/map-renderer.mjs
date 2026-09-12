// Additive Session adapter. Pass the selected-view renderWorld implementation
// from flight_opposition/ground-stars/world.patch. No world callbacks run here.
// Complete D8FF..D974 map frames have original EXE fixtures across four theaters,
// both pages, pilot/aircraft observers, heights and controls. The frame gate checks
// full video/world, display address and DS outside named private render buffers.
export function renderMapFrame(session,renderWorld,rawFrameTicks){
    const e=session.e,ds=session.ds,world=session.world;
    if(!world)throw Error('Original map requires the live DS geometry adapter');
    const u=p=>ds.getUint16(p,true),b=p=>ds.getUint8(p);
    const page=b(0xd5d)?1:0;
    e.cc_map_frame_begin(rawFrameTicks);
    e.cc_video_load(page);
    if(e.cc_epage_state_page(page))throw Error('Original map page erase failed');
    e.cc_world_horizon();
    e.cc_raster_set_seed(ds.getUint32(0xf722,true));
    const camera={position_cm:[0,4,8].map(i=>ds.getInt32(0xb05+i,true)),angles_turn65536:[0,2,4].map(i=>u(0xb11+i))};
    const report=renderWorld(e,session.snapshot,camera,{
        views:true,liveWorld:world,groundPoints:false,preserveFrame:true,objectsOnly:true,
        renderObjects(draw){
            const drawMemory=(p,horizon=false)=>{
                if(!u(p+24))return;
                if(e.cc_distance_object(p)===2)throw Error(`Original map distance failed at ${p.toString(16)}`);
                draw(world.object(p),horizon);
                // Source d3dobj clears rdf after a drawn object; threshold and
                // far early returns preserve the original special values.
                const code=b(p+18);
                if(code<3)ds.setUint8(0x205d,code===2&&ds.getUint32(p+20,true)>ds.getUint32(p+36,true)?4:0);
            };
            ds.setUint8(0x1ba9,255);drawMemory(0x1c6f,true);ds.setUint8(0x1ba9,0);
            e.cc_map_draw_begin();
            for(;;){const p=e.cc_map_draw_next();if(!p)break;if(p===0xffffffff)throw Error('Original map iterator failed');drawMemory(p);}
            drawMemory(e.cc_map_actor());
            const marker=e.cc_map_marker();if(marker)drawMemory(marker);
        }
    });
    ds.setUint32(0xf722,e.cc_raster_get_seed(),true);
    e.cc_video_store(page);
    // maploop calls drawgs unconditionally, including when the observer is a pilot.
    e.cc_cockpit_gunsight_page(page);
    session.page=e.cc_camera_flip_page();
    return report;
}

// Call only after renderMapFrame, at source mapkey. Native keyboard adapter is
// sampled once here; do not substitute FlightInput pressure for original keys.
export function updateMapControls(session,input={}){
    const e=session.e;let x=input.x??0,y=input.y??0;
    if(input.events)({x,y}=session.axes(input));
    const status=e.cc_map_controls(x,y);
    if(status)throw Error(`Original map control arithmetic returned ${status}`);
    return Boolean(e.cc_map_after_controls());
}
