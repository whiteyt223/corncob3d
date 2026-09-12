// Snapshot owns mutable point records; the core executes the original loop.
export function renderGroundStars(e,points,advance){
    const pointer=e.cc_ground_buffer(),table=new DataView(e.memory.buffer,pointer,4566),count=points.ground.length;
    if(count!==points.stars.length||count<1||count>80)throw Error('Invalid original ground/star table');
    table.setUint16(0,count,true);
    for(const [base,records] of [[2,points.ground],[0x8fa,points.stars]])for(let i=0;i<count;i++){
        const p=base+i*14,record=records[i];record.position_cm.forEach((v,axis)=>table.setInt32(p+axis*4,v,true));table.setUint16(p+12,record.fixed_flag,true);
    }
    const ds=new DataView(e.memory.buffer,e.cc_flight_state(),65536);
    ds.setUint32(0xf722,e.cc_raster_get_seed(),true);
    const status=e.cc_ground_frame(pointer,Number(advance));
    e.cc_raster_set_seed(ds.getUint32(0xf722,true));
    if(status)throw Error(`Original ground/star loop returned ${status}`);
    if(advance)for(let i=0;i<count;i++){
        const p=2+i*14;points.ground[i].position_cm=[0,4,8].map(k=>table.getInt32(p+k,true));points.ground[i].fixed_flag=table.getUint16(p+12,true);
    }
    return [...new Uint32Array(e.memory.buffer,e.cc_ground_stats(),2)];
}
