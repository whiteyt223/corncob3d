import {finishPackagedDemo} from './packaged-demos.mjs';
import {Career} from './career.mjs';
import {encodeName,decodeName} from './characters.mjs';
import {validateWorldFile} from './world-records.mjs';
import {parseAdl,SOUND_ADL_RECORD} from './sound-editor.mjs';
const u16=(b,p)=>new DataView(b.buffer,b.byteOffset,b.byteLength).getUint16(p,true);
export class Game {
    constructor(e,assets,storage){const edition=assets.editionId??0;if(storage?.edition!==undefined&&storage.edition!==edition)throw Error('Game and save editions do not match.');if(e.cc_edition_set(edition))throw Error('Unsupported original edition profile.');this.e=e;this.assets=assets;this.storage=storage;this.career=new Career(e,storage,assets.pilotFile,assets.definitions,encodeName);this.session=null;}
    async load(){await this.career.load();this.assets.config=this.career.records.get('3d.cfg');}
    definition(theater=this.career.theater()){if(!theater)return null;const stem=decodeName(theater.header.subarray(28,38)).toLowerCase();return this.assets.theaters.find(d=>d.file_stem===stem);}
    async open(index){const def=this.assets.theaters[index];if(!def)throw Error('Unknown theater.');return this.career.openTheater(this.assets.definitions.subarray(index*48,index*48+48),this.assets.worlds.get(def.file_stem));}
    airfields(){const theater=this.career.theater();if(!theater)return null;const e=this.e,p=e.cc_world_file_buffer();if(theater.bytes.length>e.cc_world_file_capacity())throw Error('Theater file is too large.');new Uint8Array(e.memory.buffer,p,theater.bytes.length).set(theater.bytes);if(e.cc_airfield_analyze_file_mode(p,theater.bytes.length,671,e.cc_edition_is_other_worlds()))throw Error('Could not read the theater airfields.');const data=new DataView(e.memory.buffer,e.cc_airfield_summary_buffer(),126),result=new Uint32Array(e.memory.buffer,e.cc_airfield_summary_result(),4);return {remaining:result[0],destroyed:result[1],single:!!result[2],rows:Array.from({length:9},(_,index)=>({index,x:data.getInt32(index*14,true),y:data.getInt32(index*14+4,true),planes:data.getUint16(index*14+8,true),objectives:data.getUint16(index*14+10,true)}))};}
    briefing(){const def=this.session?.builderDocument?{file_stem:'deftower'}:this.definition(),file=this.assets.briefings.theaters.find(t=>t.filename.toLowerCase()===`${def?.file_stem??'deftower'}.twr`);const tower=this.session?.u(0x1cb)??0;return file?.sections.find(s=>s.tower_index===tower)??{lines:[],shortcuts:[]};}
    async finish(options={}){if(this.session?.packagedDemo)return finishPackagedDemo(this);const exit=this.session?.exit;if(!exit)throw Error('No completed sortie is available.');const world=exit.world??this.career.theater()?.world??new Uint8Array();const report=await this.career.finish(exit.raw,world,{engineError:exit.engineError,config:exit.config,requestedExtraPlane:this.session.requestedExtraPlane??0,...options});if(!report.needsErrorChoice)this.assets.config=this.career.records.get('3d.cfg');return report;}
    async validateRecords(records){
        if(!records.has('pilot.scr'))throw Error('The save is missing its pilot roster.');
        if(records.has(SOUND_ADL_RECORD))parseAdl(records.get(SOUND_ADL_RECORD));
        // A separate core prevents malformed/imported content from altering the current flight.
        const {instance:{exports:e}}=await WebAssembly.instantiate(this.assets.wasm);
        if(e.cc_edition_set(this.assets.editionId??0))throw Error('Unsupported original edition profile.');
        const c=new Career(e,null,this.assets.pilotFile,this.assets.definitions,encodeName),decoded=c.decode(records.get('pilot.scr'));
        for(const pilot of decoded.pilots){
            if(pilot[0x23c]>20)throw Error('A pilot has too many open theaters.');
            // Native close-theater can leave an out-of-range selection (80h
            // decrements to7Fh). Preserve it; Operations requires a new valid selection.
            for(let i=0;i<pilot[0x23c];i++)if(!records.has(`THT/${pilot[0x23e+i]}.THT`))throw Error('The save is missing a pilot’s theater.');
        }
        for(const [name,file] of records)if(name.startsWith('THT/')){
            const id=Number(name.slice(4,-4));if(!Number.isInteger(id)||id<1||id>255)throw Error('A theater file ID is outside the original byte range.');
            if(!file.subarray(0,28).includes(0))throw Error('A theater name is invalid.');
            const stem=decodeName(file.subarray(28,38)).toLowerCase();if(!this.assets.theaters.some(t=>t.file_stem===stem))throw Error('This theater is not included in the installed edition.');
            if(file.length>e.cc_world_file_capacity())throw Error('A theater is too large.');
            validateWorldFile(file,{plain:!!e.cc_edition_is_other_worlds(),offset:671});
            new Uint8Array(e.memory.buffer,e.cc_flight_state(),65536).set(this.assets.initialDS);
            e.cc_world_allocate(0x2263);new Uint8Array(e.memory.buffer,e.cc_world_file_buffer(),file.length).set(file);
            if(e.cc_world_read_file_mode(file.length,671,e.cc_edition_is_other_worlds()))throw Error('A theater world is invalid.');
            const result=new Uint32Array(e.memory.buffer,e.cc_world_io_result(),4);if(result[1]!==file.length)throw Error('A theater contains incomplete or extra world data.');
        }
        return true;
    }
    async import(file){
        if(this.session)throw Error('Finish the current sortie before importing saves.');
        if(this.career.importing)throw Error('A save import is already in progress.');
        this.career.importing=true;
        try{
            await this.storage.import(file,async records=>{
                if(this.session)throw Error('Finish the current sortie before importing saves.');
                await this.validateRecords(records);
                if(this.session)throw Error('Finish the current sortie before importing saves.');
            });
            await this.load();
        }finally{this.career.importing=false;}
    }
}
export function pilotSummary(p){const view=new DataView(p.buffer,p.byteOffset,p.byteLength);return {name:decodeName(p.subarray(0,21)),sorties:u16(p,0x18),score:view.getInt32(0x230,true),rank:p[0x256],status:u16(p,0x16),options:p.slice(0x234,0x23c)};}
