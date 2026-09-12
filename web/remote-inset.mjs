// Original CS2C7B..2CB4, before drawtodxvec/halt. Uses selected look forward
// matrix but PILOT inverse for the fixed7000-unit display displacement.
export function renderRemoteInset(e,world,draw){
    if(!world)return false;
    const ds=new DataView(e.memory.buffer,e.cc_flight_state(),65536);
    if(!ds.getUint8(0xafc)||!ds.getUint8(0xf54))return false;
    const saved=ds.getUint16(0x1c3e,true),front=new Uint16Array(12);
    for(let i=0;i<12;i++)front[i]=ds.getUint16(saved+i*2,true);
    ds.setUint16(0x1c3e,0x1bca,true);
    // DS windows may be at odd addresses; copy through aligned scene scratch.
    const window=new Uint16Array(e.memory.buffer,e.cc_scene_vertices(),12);
    for(let i=0;i<12;i++)window[i]=ds.getUint16(0x1bca+i*2,true);
    e.cc_views_custom_front(window.byteOffset);
    try{
        e.cc_camera_erase_front();
        const octant=e.cc_camera_remote_prepare();
        draw(world.object(0x5bb0),false,[0,0,0,octant,0]);
    }finally{
        ds.setUint16(0x1c3e,saved,true);window.set(front);e.cc_views_custom_front(window.byteOffset);
    }
    return true;
}
