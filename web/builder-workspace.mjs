import {createFlight} from './startup.mjs';
import {validateWorldFile,readWorldDefinition} from './world-records.mjs';
import {LiveWorld} from './live-world.mjs';
import {renderWorld} from './world.mjs';
import {renderBuilderScene} from './builder-renderer.mjs';
export function definitionName(name){const base=String(name??'CUSTOM.DEF').split(/[\\/]/).pop().replace(/[^a-zA-Z0-9_. -]/g,'_');return (base.replace(/\.def$/i,'')||'CUSTOM')+'.DEF';}
// Browser replacement for supplying 3UNIV.DAT to the documented `3 -rs`
// command. It uses the actual startup switches and leaves pilot data alone.
export function createBuilderFlight(e,assets,bytes,{name='CUSTOM.DEF',...options}={}){
    if(assets.editionId!==2)throw Error('The mission builder requires Other Worlds.');
    validateWorldFile(bytes,{plain:true});
    const created=createFlight(e,assets,null,{...options,extraPlane:false,editorWorld:bytes.slice(),demoPlayback:null,packagedDemo:null});
    const ready=session=>{session.builderDocument={name:definitionName(name)};return session;};
    return created?.then?created.then(ready):ready(created);
}
// Take a coherent editing snapshot before any async work. Original mbguts
// clears its cursor before flushtiles; original exit then writes the plain
// world plus the unchanged 12-byte starting-location tail. A second core lets
// Save leave the current editor frame and its object tables intact.
export async function exportBuilderDefinition(session,wasm){
    if(!session?.builderDocument)throw Error('No mission-builder document is open.');
    if(session.exit?.world){const bytes=session.exit.world.slice();validateWorldFile(bytes,{plain:true});return {name:session.builderDocument.name,bytes};}
    const source=session.e,active=!!source.cc_builder_active();
    const ds=new Uint8Array(source.memory.buffer,source.cc_flight_state(),65536).slice();
    const world=new Uint8Array(source.memory.buffer,source.cc_universe_memory(),0x45f90).slice();
    const video=new Uint8Array(source.memory.buffer,source.cc_video_memory(),524288).slice(),runtime=new Uint32Array(source.memory.buffer,source.cc_runtime_state(),3).slice(),snapshot=structuredClone(session.snapshot);
    const {instance:{exports:e}}=await WebAssembly.instantiate(wasm);
    if(e.cc_edition_set(2))throw Error('Other Worlds is unavailable.');
    new Uint8Array(e.memory.buffer,e.cc_flight_state(),ds.length).set(ds);
    new Uint8Array(e.memory.buffer,e.cc_universe_memory(),world.length).set(world);
    const copyDS=new DataView(e.memory.buffer,e.cc_flight_state(),65536);
    if(active){
        new Uint8Array(e.memory.buffer,e.cc_video_memory(),video.length).set(video);new Uint32Array(e.memory.buffer,e.cc_runtime_state(),3).set(runtime);
        copyDS.setUint8(0x26c,0);for(let i=0;i<3;i++)copyDS.setUint16(0x1da8+i*2,0,true);copyDS.setUint16(0xff10+24,0,true);
        const copy={e,ds:copyDS,snapshot,world:new LiveWorld(e,snapshot,new Uint8Array(),new Uint8Array())};renderBuilderScene(copy,renderWorld,0);
    }
    if(e.cc_flush_tiles())throw Error('The original editor could not flush the active region.');
    if(e.cc_world_write_file_mode(0,1))throw Error('The original editor could not write the definition.');
    const result=new Uint32Array(e.memory.buffer,e.cc_world_io_result(),4),bytes=new Uint8Array(e.memory.buffer,e.cc_world_file_buffer(),result[1]).slice();
    validateWorldFile(bytes,{plain:true});
    return {name:session.builderDocument.name,bytes};
}
export function downloadDefinition({name,bytes},{document=globalThis.document,URL=globalThis.URL}={}){
    const url=URL.createObjectURL(new Blob([bytes],{type:'application/octet-stream'})),link=document.createElement('a');link.href=url;link.download=definitionName(name);link.click();setTimeout(()=>URL.revokeObjectURL(url),1000);
}
// This is a file-launch adapter, separate from MOAG's career/theater commands.
export function builderOptions(menu){
    const doc=menu.root.ownerDocument,assets=menu.game.assets;if(assets.editionId!==2)throw Error('The mission builder requires Other Worlds.');
    menu.screen('Mission builder');
    const description=doc.createElement('p');description.textContent='Open a definition to edit its world. Press M for the map, then B to enter the builder.';menu.content.append(description);
    const label=doc.createElement('label');label.textContent='Definition ';const select=doc.createElement('select');
    for(const [stem]of assets.worlds){const option=doc.createElement('option');option.value=stem;option.textContent=stem.toUpperCase()+'.DEF';select.append(option);}label.append(select);menu.content.append(label);
    const upload=doc.createElement('input');upload.type='file';upload.accept='.def';upload.hidden=true;menu.content.append(upload);
    upload.addEventListener('change',()=>{const file=upload.files?.[0];upload.value='';if(file)void menu.run(async()=>menu.actions.openBuilder(await readWorldDefinition(file),file.name));});
    menu.nav([['Open definition',()=>menu.actions.openBuilder(assets.worlds.get(select.value),select.value+'.DEF')],['Import definition',()=>upload.click()],['Operations',()=>menu.hangar()]]);
    const details=doc.createElement('details'),summary=doc.createElement('summary'),text=doc.createElement('pre');summary.textContent='Original builder keys';
    text.textContent='M / Esc: return to map\nLeft Shift: show cursor · Left Ctrl: hide cursor\nSpace: select nearest · keypad − / +: previous / next nearby object\nC: create object · D: delete selected object\n− / =: object type · F1–F8: quality 1–8\nF9: special flag · F10: completed flag\nLeft Alt + directional controls: move selected object\nA + directional controls: rotate selected object · A + keypad 5: snap angles\nPage Up / Down: height · S: fine height steps\nG: cursor to ground · H: apply stored height · Left Alt+H: store height\n1–0: movement speed · keypad5 / Insert: forward / backward\nCaps Lock: level view';
    details.append(summary,text);menu.content.append(details);
}
export function builderSaveHandler({game,menu,assets,canvas,isPaused,pause,isFinishing,canResume=()=>true,download=downloadDefinition,exportFile=exportBuilderDefinition}){
    let saving=false;
    return async()=>{
        if(!game.session?.builderDocument||!menu.root.hidden||menu.busy||saving||isFinishing())return false;
        const session=game.session,resume=!isPaused();pause(true);
        const restore=()=>{menu.hide();if(resume&&canResume())pause(false);canvas.focus();};
        const attempt=async()=>{
            saving=true;menu.screen('Exporting definition');
            try{download(await exportFile(session,assets.wasm));restore();return true;}
            catch(error){menu.screen('Definition was not exported');menu.message.textContent=error.message;menu.nav([['Retry export',attempt],['Return to editor',restore]]);return false;}
            finally{saving=false;}
        };
        return attempt();
    };
}
