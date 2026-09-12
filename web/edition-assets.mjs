import {loadPackagedDemos} from './packaged-demos.mjs';
import {HELP_IDS,decodeHelpImages} from './help.mjs';

// Each packaged edition declares its own original assets. Shared files appear
// in both manifests only after their original bytes have been compared.
export async function loadEditionAssets({search='',binary,json}){
    const editions=await json('editions.json'),key=new URLSearchParams(search).get('edition')??editions[0]?.key;
    const profile=editions.find(item=>item.key===key);
    if(!profile)throw Error('This Corncob edition is not installed.');
    const files=profile.files;
    const [wasm,snapshot,initialDS,cockpit,pilotFile,definitions,theaterData,briefings,groundTables,modePalette,soundAdl,defaultSoundAdl]=await Promise.all([
        binary(files.wasm),json(files.snapshot),binary(files.initialDS),binary(files.cockpit),binary(files.pilotFile),binary(files.definitions),json(files.theaters),json(files.briefings),binary(files.groundTables),binary(files.modePalette),binary(files.soundAdl),binary(files.defaultSoundAdl??files.soundAdl)
    ]);
    const worlds=new Map(await Promise.all(theaterData.records.map(async def=>[def.file_stem,await binary(profile.worlds[def.file_stem])])));
    const towers=new Map(await Promise.all(Object.entries(profile.towers).map(async([stem,url])=>[stem,await binary(url)])));
    const {instance:{exports:e}}=await WebAssembly.instantiate(wasm);
    const assets={wasm,snapshot,initialDS,cockpit,pilotFile,definitions,theaters:theaterData.records,briefings,worlds,towers,groundTables,modePalette,soundAdl,defaultSoundAdl,editionId:profile.id,edition:profile,editions};
    assets.resultBase=files.resultScenes.slice(0,files.resultScenes.lastIndexOf('/')+1);
    assets.titleBase=files.titleScenes.slice(0,files.titleScenes.lastIndexOf('/')+1);
    [assets.pilotLabels,assets.resultScenes,assets.titleScenes]=await Promise.all([json(files.pilotLabels),json(files.resultScenes),json(files.titleScenes)]);
    assets.helpImages=decodeHelpImages(e,new Map(await Promise.all(HELP_IDS.map(async id=>[id,await binary(profile.images[id])]))));
    assets.packagedDemos=await loadPackagedDemos(files.demoCatalog,{json,binary});
    return {e,assets};
}
