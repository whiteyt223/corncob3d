import {readFile} from 'node:fs/promises';
import {HELP_IDS,decodeHelpImages} from '../web/help.mjs';
import {Game} from '../web/game.mjs';
const root=new URL('../',import.meta.url),read=p=>readFile(new URL(p,root)),json=async p=>JSON.parse(await read(p));
export async function loadGameFixture(){
    const wasm=await read('build/corncob-math.wasm'),theaterData=await json('docs/theater-definitions-original.json'),theaters=theaterData.records;
    const assets={wasm,theaters,snapshot:await json('docs/world-runtime.json'),initialDS:await read('reference/original-initial-ds.bin'),cockpit:await read('reference/original-cockpit.indices'),pilotFile:await read('reference/captured-pilot.scr'),definitions:await read('reference/theater-definitions-original.bin'),briefings:await json('docs/briefings-original.json'),groundTables:await read('reference/original-ground-tables.bin'),modePalette:await read('reference/original-mode10-palette.bin'),worlds:new Map(),towers:new Map()};
    for(const def of theaters)assets.worlds.set(def.file_stem,await read(`originals/shareware-v342/extracted/${def.file_stem.toUpperCase()}.CCT`));
    for(const stem of [...theaters.map(d=>d.file_stem),'deftower'])assets.towers.set(stem,await read(`originals/shareware-v342/extracted/${stem.toUpperCase()}.TWR`));
    const {instance:{exports:e}}=await WebAssembly.instantiate(wasm);assets.helpImages=decodeHelpImages(e,new Map(await Promise.all(HELP_IDS.map(async id=>[id,await read(`originals/shareware-v342/extracted/3D${id}.IMG`)]))));let records=new Map();const storage={edition:0,records:async()=>new Map([...records].map(([k,v])=>[k,v.slice()])),commit:async(entries,{replace=false}={})=>{if(replace)records=new Map();for(const [k,v] of entries)records.set(k,v.slice());}};
    const game=new Game(e,assets,storage);await game.load();return {e,assets,game,storage};
}
