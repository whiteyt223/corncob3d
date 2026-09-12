// Original runtime graph, original object units, and the portable integer core.
// This connects rendering paths; object callbacks/progression are added by the
// simulation separately. A sampled graph is not a whole-frame parity fixture.
import {regenerateGroundPoint} from './world-dynamics.mjs';
import {renderGroundStars} from './ground-stars.mjs';
export function renderWorld(e,snapshot,camera,{cockpit,groundPoints=true,preserveFrame=false,advanceGround=false,liveWorld,renderObjects,afterObjects,objectsOnly=false,views=false}={}){
    const memory=e.memory.buffer,words=(p,n)=>new Uint16Array(memory,p,n),longs=(p,n)=>new Uint32Array(memory,p,n);
    const frame=new Uint8Array(memory,e.cc_framebuffer(),640*350);
    if(cockpit)frame.set(cockpit);else if(!preserveFrame)e.cc_clear(0);
    const fill=color=>{for(let y=4;y<199;y++)frame.fill(color,y*640+8,y*640+632);};
    const ds=new DataView(memory,e.cc_flight_state(),65536);let rear=false;
    const syncViews=()=>{
        if(!views)return;rear=ds.getUint16(0x1c40,true)!==0;
        e.cc_views_configure(Number(ds.getUint16(0x1c3e,true)===0x1c26),Number(rear));
        e.cc_views_fill(liveWorld?ds.getUint8(0x1baf):1);
        if(liveWorld){const p=ds.getUint16(0x1c3e,true),window=words(e.cc_scene_vertices(),12);for(let i=0;i<12;i++)window[i]=ds.getUint16(p+i*2,true);e.cc_views_custom_front(window.byteOffset);}
    };
    const upper=liveWorld?ds.getUint8(0x2c41):11,lower=liveWorld?ds.getUint8(0x2c42):2;
    let dvflag=-(((camera.angles_turn65536[1]<<16)>>24)>>1);
    if(views){syncViews();e.cc_views_flags(0,dvflag&255,upper,lower);}else fill(11);
    const viewStatus=(status)=>{if(status)throw Error(`Original viewport wrapper returned ${status}`);};
    const report={objects:0,polygons:0,discs:0,wires:0,points:0,rejected:0,unsupported:0};
    const prepare=object=>{
        longs(e.cc_world_input(),16).set([...camera.position_cm,...camera.angles_turn65536,...object.position_cm,...object.angles_turn65536,...object.distance_squared_breakpoints,object.rotation_cutoff]);
        if(views&&liveWorld?e.cc_prepare_object_view(object.type_status):e.cc_prepare_object())return null;
        return [...longs(e.cc_world_output(),5)];
    };
    const setVertices=(vertices,rdf=0)=>words(e.cc_scene_vertices(),60).set(vertices.flat().map(v=>v>>rdf));
    const color=c=>c.index;
    const draw=(object,horizon=false,directPrepared=null)=>{
        if(!directPrepared&&!object.type_status)return;
        syncViews();const prepared=directPrepared??prepare(object);if(!prepared){report.rejected++;return;}
        const [,rdf,,octant,slot]=prepared,mesh=liveWorld&&(!horizon||views)?liveWorld.mesh(object.mesh_pointers[slot]):snapshot.meshes[object.mesh_pointers[slot]];
        if(!mesh){report.unsupported++;return;}
        report.objects++;
        for(const reference of mesh.orders[octant]??[]){
            if(views)ds.setUint8(0x1fc7,0); // drrobj resets before every slot, including kind0.
            if(!reference.kind)continue;
            const part=liveWorld&&(!horizon||views)?liveWorld.component(reference):snapshot.components[`${reference.kind}:${reference.component_address}`];
            if(!part){report.unsupported++;continue;}
            let failed=0;
            if(views){
                syncViews();e.cc_views_flags(Number(horizon),dvflag&255,upper,lower);
                if(part.kind===8){setVertices(part.vertices,rdf);viewStatus(e.cc_render_views_polygon(part.count,color(part.color),rdf));report.polygons++;}
                else if(part.kind===6)for(const disc of part.discs){setVertices([disc.center],rdf);viewStatus(e.cc_render_views_disc((disc.diameter&65535)>>>rdf,color(disc.color)));report.discs++;}
                else if(part.kind===4)for(const point of part.points){setVertices([point.center],rdf);viewStatus(e.cc_render_views_point(color(point.color)));report.points++;}
                else if(part.kind===2){
                    for(const wire of part.wires)for(let i=0;i<wire.segments;i++){setVertices(wire.vertices.slice(i,i+2),rdf);viewStatus(e.cc_render_views_wire(color(wire.color),rdf));report.wires++;}
                    ds.setUint8(0x1fc7,0);
                }
                e.cc_views_flags(0,dvflag&255,upper,lower);
                if(rdf&&(rear?ds.getUint8(0x1fc7)===2:ds.getUint8(0x1fc7)!==0))return;
                continue;
            }
            if(part.kind===8){setVertices(part.vertices,rdf);const status=e.cc_render_polygon(part.count,color(part.color));report.polygons++;if(status)report.rejected++;}
            else if(part.kind===6){
                for(const disc of part.discs){setVertices([disc.center],rdf);const status=e.cc_render_disc((disc.diameter&65535)>>>rdf,color(disc.color));report.discs++;if(status){report.rejected++;failed++;}}
            }else if(part.kind===4){
                for(const point of part.points){setVertices([point.center],rdf);const status=e.cc_render_point(color(point.color));report.points++;if(status){report.rejected++;failed++;}}
            }else if(part.kind===2){
                for(const wire of part.wires)for(let i=0;i<wire.segments;i++){
                    setVertices(wire.vertices.slice(i,i+2),rdf);
                    const status=horizon?e.cc_prepare_wire(rdf):e.cc_render_wire(color(wire.color),rdf);report.wires++;
                    if(horizon){
                        const pitchByte=(camera.angles_turn65536[1]<<16)>>24,dvflag=-(pitchByte>>1);
                        if(status)fill(dvflag<0?2:11);
                        else{const p=[...new Int16Array(memory,e.cc_line_output(),4)];if(e.cc_draw_horizon(...p,11,2))fill(dvflag<0?2:11);}
                    }else if(status)report.rejected++;
                }
            }
            if(rdf&&failed)return;
        }
    };
    if(objectsOnly){renderObjects?.(draw,report);return report;}
    const hm=(a,b)=>(a*b)>>15;
    if(views&&liveWorld){
        e.cc_world_horizon();dvflag=ds.getUint8(0x205e);ds.setUint8(0x1ba9,255);
        draw(liveWorld.object(0x1c6f),true);ds.setUint8(0x1ba9,0);ds.setUint8(0x205d,0);
    }else{
    // modhr uses the camera's inverse matrix to keep a directed horizon wire.
    e.cc_scene_angles(...camera.angles_turn65536,1);
    const inverse=[...new Int16Array(memory,e.cc_scene_matrix(),9)];
    const a=hm(1300,inverse[0]),b=hm(1300,inverse[3]);
    const horizon={...snapshot.special.horizon.object,position_cm:camera.position_cm.map((v,i)=>(v+(i===0?a:i===1?b:0))|0)};
    const horizonMesh=snapshot.meshes[horizon.mesh_pointers[0]],horizonRef=horizonMesh.orders[0].find(r=>r.kind===2);
    const horizonPart=snapshot.components[`2:${horizonRef.component_address}`];
    // Private copy avoids mutating the preserved capture data.
    const prior=horizonPart.wires[0].vertices;horizonPart.wires[0].vertices=[[-b,a,0],[b,-a,0]];
    draw(horizon,true);horizonPart.wires[0].vertices=prior;
    }
    if(views&&ds.getUint8(0xd5c)){afterObjects?.(draw);return report;}
    if(groundPoints&&views&&liveWorld){
        const [attempted,rejected]=renderGroundStars(e,snapshot.special.ground_points,advanceGround);report.points+=attempted;report.rejected+=rejected;
    }else if(groundPoints&&(liveWorld?ds.getUint8(0x1bac):snapshot.special.ground_points.ground_enabled)){
        const random={nextWord(){const seed=Math.imul(e.cc_raster_get_seed(),663608941)>>>0;e.cc_raster_set_seed(seed);return seed>>>16;}};
        for(const point of snapshot.special.ground_points.ground){
            let successful=false;
            const prepared=prepare({position_cm:point.position_cm,angles_turn65536:[0,0,0],distance_squared_breakpoints:[0xffffffff,0xffffffff,0xffffffff],rotation_cutoff:0});
            if(prepared){
                const relative=point.position_cm.map((v,i)=>((v-camera.position_cm[i])|0)>>(prepared[1]+1));
                setVertices([relative]);e.cc_pmvmul(e.cc_scene_matrix(),e.cc_scene_vertices(),e.cc_scene_origin());
                setVertices([[0,0,0]]);
                if(views&&point.fixed_flag){viewStatus(e.cc_render_views_point((liveWorld?ds.getUint8(0x1bae):snapshot.special.ground_points.color_index)));const result=longs(e.cc_views_result(),6);successful=(rear?result[1]:result[0])===0;}
                else successful=e.cc_render_point((liveWorld?ds.getUint8(0x1bae):snapshot.special.ground_points.color_index))===0;if(!successful)report.rejected++;report.points++;
            }
            if(advanceGround){if(!successful)regenerateGroundPoint(point,camera.position_cm,random);if(random.nextWord()<=91)regenerateGroundPoint(point,camera.position_cm,random);}
        }
    }
    // Sun origin is a direction vector; no observer-position subtraction.
    const sun=snapshot.special.sun,angle=liveWorld?ds.getUint16(0x280,true):sun.sunangle;
    const sun1=liveWorld?ds.getUint16(0x5386,true):150,sun2=liveWorld?ds.getUint16(0x5390,true):155,sunColor=liveWorld?ds.getUint8(0x537d):sun.sun_color_index;
    e.cc_scene_angles(...camera.angles_turn65536,0);
    words(e.cc_scene_vertices(),60).set([0,hm(-e.cc_ssin(angle),1000),hm(e.cc_scos(angle),1000)]);
    e.cc_matvmul(e.cc_scene_matrix(),e.cc_scene_vertices(),e.cc_scene_origin());
    words(e.cc_scene_vertices(),60).fill(0);
    if(!liveWorld||!(ds.getUint16(0x1de,true)&0x2000)){
        if(views){viewStatus(e.cc_render_views_disc(sun1,15));viewStatus(e.cc_render_views_disc(sun2,sunColor));}
        else{e.cc_render_disc(sun1,15);e.cc_render_disc(sun2,sunColor);}
    }
    // Sun's d3dobj path refreshes local rotation to zero before ordinary objects.
    e.cc_scene_angles(0,0,0,1);words(e.cc_world_local_matrix(),9).set(words(e.cc_scene_matrix(),9));
    if(renderObjects)renderObjects(draw,report);
    else if(liveWorld)liveWorld.renderSnapshot(draw);
    else{
        for(const object of snapshot.medfar_objects??[])draw(object);
        const near=snapshot.near_table.objects;
        const runway=o=>[5,35,36].includes(o.template_index)&&o.position_cm[2]===0;
        for(const object of near.filter(runway))draw(object);
        for(const object of near.filter(o=>!runway(o)))draw(object);
    }
    if(afterObjects)afterObjects(draw);
    return report;
}
export function worldRGBA(e,snapshot){
    const frame=new Uint8Array(e.memory.buffer,e.cc_framebuffer(),640*350),out=new Uint8ClampedArray(frame.length*4),palette=snapshot.palette.render_rgb8;
    for(let i=0;i<frame.length;i++){const c=palette[frame[i]&15];out[i*4]=c[0];out[i*4+1]=c[1];out[i*4+2]=c[2];out[i*4+3]=255;}
    return out;
}
