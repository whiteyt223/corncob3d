// Browser and offline inspection share the same Wasm calls. No substitute
// JavaScript rasterizer or floating-point projection is used.
export const diagnosticPalette=[0x000000,0x0000aa,0x00aa00,0x00aaaa,0xaa0000,0xaa00aa,0xaa5500,0xaaaaaa,0x555555,0x5555ff,0x55ff55,0x55ffff,0xff5555,0xff55ff,0xffff55,0xffffff];
export function renderModel(e,geometry,{mesh,order=0,yaw=8192,pitch=4096,distance=16000}){
    const model=geometry.meshes[mesh];if(!model)throw Error('Unknown original mesh');
    e.cc_clear(7);e.cc_scene_angles(yaw,pitch,0,1);
    // Model origin in the source is overwritten by the runtime's relative
    // position. This isolated fixture supplies it explicitly.
    new Uint16Array(e.memory.buffer,e.cc_scene_origin(),3).set([distance,0,0]);
    const report={polygons:0,discs:0,rejected:0,unsupported:0,dynamicColors:0};
    const color=value=>{if(typeof value==='number'&&value>=0&&value<16)return value;report.dynamicColors++;return 0;};
    for(const ref of model.orders[order]??[]){
        if(ref.kind===0)continue;
        const component=geometry.components[`${ref.kind}:${ref.symbol}`];
        if(!component)throw Error(`Missing original component ${ref.symbol}`);
        if(ref.kind===8){
            new Uint16Array(e.memory.buffer,e.cc_scene_vertices(),60).set(component.vertices.flat());
            if(e.cc_render_polygon(component.count,color(component.color)))report.rejected++;
            report.polygons++;
        }else if(ref.kind===6){
            for(const disc of component.discs){
                new Uint16Array(e.memory.buffer,e.cc_scene_vertices(),60).set(disc.center);
                if(e.cc_render_disc(disc.diameter,color(disc.color)))report.rejected++;
                report.discs++;
            }
        }else report.unsupported++;
    }
    return report;
}
export function rgbaFrame(e,height=200){
    const indices=new Uint8Array(e.memory.buffer,e.cc_framebuffer(),640*height),rgba=new Uint8ClampedArray(indices.length*4);
    for(let i=0;i<indices.length;i++){
        const rgb=diagnosticPalette[indices[i]&15];rgba[i*4]=rgb>>16;rgba[i*4+1]=(rgb>>8)&255;rgba[i*4+2]=rgb&255;rgba[i*4+3]=255;
    }
    return rgba;
}
