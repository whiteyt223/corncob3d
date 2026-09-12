// Original packed DS records and mutable geometry. This adapter interprets
// original memory; it contains no replacement objects or gameplay formulas.
export class LiveWorld {
    constructor(e,snapshot,initial,tiles){
        this.e=e;this.snapshot=snapshot;this.initial=new Uint8Array(initial);this.tiles=new Uint8Array(tiles);
        this.ds=new DataView(e.memory.buffer,e.cc_flight_state(),65536);
        this.cache=new DataView(e.memory.buffer,e.cc_universe_memory(),0x45f90);
    }
    reset(){
        const bytes=new Uint8Array(this.cache.buffer,this.cache.byteOffset,this.cache.byteLength);bytes.fill(0);
        if(this.initial.length>=196608)bytes.set(this.initial.subarray(131072,196608));
        bytes.set(this.tiles,0xff90);
    }
    object(p,m=this.ds){
        const words=(at,n)=>Array.from({length:n},(_,i)=>m.getUint16(p+at+i*2,true));
        return {address:p,position_cm:[0,4,8].map(i=>m.getInt32(p+i,true)),angles_turn65536:words(12,3),
            distance_code:m.getUint8(p+18),distance_squared:m.getUint32(p+20,true),type_status:m.getUint16(p+24,true),
            rotation_cutoff:m.getUint8(p+26),damage:m.getUint8(p+27),distance_squared_breakpoints:[28,32,36].map(i=>m.getUint32(p+i,true)),
            mesh_pointers:words(40,3),velocity:words(46,3),time:m.getUint16(p+52,true),template_index:m.getUint16(p+72,true)};
    }
    color(word){return {word,index:word<256?word:this.ds.getUint8(word)};}
    component({kind,component_address:p}){
        const m=this.ds,u=at=>m.getUint16(at,true),xyz=at=>[0,2,4].map(i=>m.getInt16(at+i,true)),count=u(p);
        if(count>=512)throw Error(`Invalid original component at ${p}`);
        if(kind===8)return {kind,count,color:this.color(u(p+2)),vertices:Array.from({length:count},(_,i)=>xyz(p+4+i*6))};
        if(kind===2)return {kind,count,wires:Array.from({length:count},(_,i)=>{
            const q=u(p+2+i*2),segments=u(q);if(segments>=512)throw Error(`Invalid original wire at ${q}`);
            return {segments,color:this.color(u(q+2)),vertices:Array.from({length:segments+1},(_,j)=>xyz(q+4+j*6))};
        })};
        const stride=kind===4?8:10,items=Array.from({length:count},(_,i)=>{
            const q=p+2+i*stride;return {center:xyz(q),...(kind===6?{diameter:u(q+6)}:{}),color:this.color(u(q+stride-2))};
        });
        return {kind,count,[kind===4?'points':'discs']:items};
    }
    mesh(p){
        const m=this.ds,count=m.getUint16(p+6,true);
        if(count>=256)throw Error(`Invalid original mesh at ${p}`);
        return {orders:Array.from({length:8},(_,octant)=>Array.from({length:count},(_,i)=>{
            const q=p+8+(octant*count+i)*4;return {kind:m.getUint16(q,true),component_address:m.getUint16(q+2,true)};
        }))};
    }
    cached(){const result=[];for(let p=0;p<0xff88;p+=74)if(this.cache.getUint16(p+24,true))result.push(this.object(p,this.cache));return result;}
    active(){const table=this.ds.getUint16(0x2393,true),n=this.ds.getUint16(table,true);return Array.from({length:n},(_,i)=>this.ds.getUint16(table+4+i*2,true));}
    renderSnapshot(draw){for(const o of this.cached())draw(o);const near=this.active().map(p=>this.object(p));const runway=o=>[5,35,36].includes(o.template_index)&&o.position_cm[2]===0;for(const o of near.filter(runway))draw(o);for(const o of near.filter(o=>!runway(o)))draw(o);}
    renderFrame(draw,{nearOnly=false}={}){
        const e=this.e,m=this.ds,u=p=>m.getUint16(p,true),b=p=>m.getUint8(p),w=(p,v)=>m.setUint16(p,v,true),byte=(p,v)=>m.setUint8(p,v);
        const seedOut=()=>m.setUint32(0xf722,e.cc_raster_get_seed(),true),seedIn=()=>e.cc_raster_set_seed(m.getUint32(0xf722,true));
        let restoreSequence=e.cc_weapon_restore_sequence();
        const check=(status,phase,p)=>{const current=e.cc_weapon_restore_sequence();if(current!==restoreSequence){restoreSequence=current;e.cc_video_load(u(0x1b90)===0?1:0);}if(status){this.failure={status,phase,object:p};throw Error(`Original ${phase} returned ${status}${p===undefined?'':` at ${p.toString(16)}`}`);}};
        const drawMemory=p=>{
            if(!u(p+24))return;
            check(e.cc_distance_object(p)===2?2:0,'distance',p);seedIn();draw(this.object(p));seedOut();
            // d3dobj resets rdf after drrobj; its early returns preserve the
            // prior value, except code2 threshold rejection has already set4.
            const code=b(p+18);if(code<3)byte(0x205d,code===2&&m.getUint32(p+20,true)>m.getUint32(p+36,true)?4:0);
        };
        const firstRunway=p=>{if(m.getUint32(p+8,true)!==0)return false;const type=u((0x2d44+u(p+72)*2)&65535);w(0x1e66,type);return Boolean(type&16);};
        const contact=p=>{if(b(0xafc))check(e.cc_contact_walk(p),'pilot contact',p);check(e.cc_contact_crash(p),'aircraft contact',p);};
        seedOut();this.failure=null;
        // Far entries draw and promote in address order. A promoted record can
        // subsequently draw again in the near pass during this same frame.
        const bytes=new Uint8Array(m.buffer,m.byteOffset,m.byteLength),cacheBytes=new Uint8Array(this.cache.buffer,this.cache.byteOffset,65536);
        if(!nearOnly){
        for(let p=0;p<0xff88;p+=74)if(this.cache.getUint16(p+24,true)){
            bytes.set(cacheBytes.subarray(p,p+74),0x2159);
            if(b(0xf4e))drawMemory(0x2159);else e.cc_distance_object(0x2159);
            check(e.cc_promote_cached(p),'activation',p);
        }
        e.cc_audio_ambient_frame(u(0xf0d1),b(0xf0d3),u(0xf0c9),b(0xf0cd),u(0xf0ce),b(0xf0d0));
        check(e.cc_enemy_initnumbers(),'population update');check(e.cc_move_bullets(),'bullet movement');check(e.cc_move_effects(),'object movement');
        byte(0xf53,0);m.setInt32(0x2bb,0,true);m.setInt32(0x2bf,0,true);
        }
        const table=u(0x2393);w(0x2156,0);
        for(let slot=table+4,n=u(table);n;n--,slot+=2){const p=u(slot);if(u(p+24)){w(0x2156,u(0x2156)+1);if(firstRunway(p)){drawMemory(p);contact(p);}}}
        if(u(0x2156)>u(0x27e))w(0x27e,u(0x2156));
        w(0x2955,table);let slot=table+4,remaining=u(table);const hadObjects=remaining!==0;
        while(remaining){
            const p=u(slot);w(0x2957,b(p+18));
            if(u(p+24)&16)check(e.cc_enemy_timed(p),'timed callback',p);
            if(remaining>1&&!u(p+24))check(e.cc_contact_crash(p),'inactive aircraft contact',p);
            else{
                if(remaining>1)check(e.cc_contact_pairs(p,slot),'object collision',p);
                if(firstRunway(p))byte(0x205d,b(p+18));else{drawMemory(p);contact(p);}
            }
            if(!u(p+24)){check(e.cc_table_remove(table,p),'table removal',p);w(0x2955,table);}else slot+=2;
            remaining--;
        }
        if(hadObjects)check(e.cc_table_sort(table),'table sort');
        if(!nearOnly){check(e.cc_doxp()===2?2:0,'portal relocation');check(e.cc_donewmiss()===2?2:0,'mission relocation');
        check(e.cc_cache_far_objects(),'world cache');e.cc_update_tiles();}seedIn();
        // Native cecode is consumed by the source late-frame sortie gate.
        // API status failures above remain explicit host integration errors.
    }
}
