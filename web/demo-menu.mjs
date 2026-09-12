import {packagedDemoOptions} from './packaged-demos.mjs';
function paragraph(text){const p=document.createElement('p');p.textContent=text;return p;}
export function demoOptions(menu,library){
    if(!library.enabled)throw Error('Demo recording and playback are available in Other Worlds.');
    const box=menu.screen('Other Worlds demos');
    box.append(paragraph('Release U during flight to begin recording; release U again to finish 3demo.dat.'));
    box.append(paragraph('A demo stores movement and action records. Select the world in which you want to replay it.'));
    if(library.imported)box.append(paragraph(`Imported: ${library.name}. Replay on next sortie: ${library.nextReplay?'on':'off'}.`));
    if(library.recording)box.append(paragraph(`Recording: ${library.recording.byteLength} bytes${library.recording.complete?', complete':', still open'}.`));
    const choose=()=>{
        const input=document.createElement('input');input.type='file';input.accept='.dat';input.hidden=true;box.append(input);
        input.addEventListener('change',()=>{const file=input.files?.[0];input.remove();if(file)void menu.run(async()=>{await library.importFile(file);demoOptions(menu,library);});},{once:true});
        input.addEventListener('cancel',()=>input.remove(),{once:true});input.click();
    };
    const download=()=>{
        const bytes=library.exportRecording(),url=URL.createObjectURL(new Blob([bytes],{type:'application/octet-stream'}));
        const a=document.createElement('a');a.href=url;a.download='3demo.dat';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);
    };
    const actions=[];if(menu.game.assets.packagedDemos?.catalog.records.length)actions.push(['Watch an original demo flight',()=>packagedDemoOptions(menu)]);actions.push(['Import demo',choose]);
    if(library.imported)actions.push([library.nextReplay?'Cancel next replay':'Replay on next sortie',()=>{library.nextReplay=!library.nextReplay;demoOptions(menu,library);}]);
    if(library.recording?.byteLength)actions.push(['Export 3demo.dat',download]);
    actions.push(['Operations',()=>menu.hangar()]);menu.nav(actions);
}
