/*
 * User-imported ROL/BNK controller.  It deliberately fetches no music asset:
 * callers pass File objects from an explicit picker/input.
 */
import {RolMusic} from './rol-music.mjs';

export const SOURCE_PLAYLISTS=Object.freeze({
    title:Object.freeze(['DESTINY3.ROL']),
    moag:Object.freeze(['RIDINGT.ROL','DREAMSO2.ROL']),
});
export const SOURCE_MUSIC_PROFILES=Object.freeze({
    0:Object.freeze({bank:'CCMUSIC.BNK',playlists:SOURCE_PLAYLISTS}),
    2:Object.freeze({bank:'BNK835.BNK',playlists:Object.freeze({
        title:Object.freeze(['VOYAGE3.ROL']),
        moag:Object.freeze(['HEAVENS.ROL','DAYBREAK.ROL']),
    })}),
});
const ROL_MAX_BYTES=16*1024*1024,BNK_MAX_BYTES=16*1024*1024;
const MAX_FILES=33,MAX_COLLECTION_BYTES=32*1024*1024,MAX_COLLECTION_EVENTS=100000;
const upper=name=>name.replace(/^.*[\\/]/,'').toUpperCase();
const bytes=value=>value instanceof Uint8Array?value:new Uint8Array(value);
function invalid(message){throw new TypeError(message);}
function rolHeader(data,name){
    const b=bytes(data);if(b.byteLength<203||b.byteLength>ROL_MAX_BYTES)invalid(`${name}: invalid ROL size`);
    const d=new DataView(b.buffer,b.byteOffset,b.byteLength);
    if(d.getUint16(0,true)!==0||d.getUint16(2,true)!==4)invalid(`${name}: expected ROL version 0.4`);
}
function bnkHeader(data,name){
    const b=bytes(data);if(b.byteLength<20||b.byteLength>BNK_MAX_BYTES)invalid(`${name}: invalid BNK size`);
    if(String.fromCharCode(...b.subarray(2,8))!=='ADLIB-')invalid(`${name}: expected ADLIB- BNK signature`);
    const d=new DataView(b.buffer,b.byteOffset,b.byteLength),total=d.getUint16(10,true),names=d.getInt32(12,true),records=d.getInt32(16,true);
    if(total===0||names<0||records<0||names+total*12>b.byteLength||records>b.byteLength)invalid(`${name}: invalid BNK index`);
}
async function readFile(file){
    if(!file||typeof file.name!=='string'||typeof file.arrayBuffer!=='function')invalid('expected a user-selected File');
    const data=new Uint8Array(await file.arrayBuffer());return {name:upper(file.name),data};
}
/**
 * Uses the existing `bridge.playRol(rolUrl, bnkUrl, {loop})` and
 * `bridge.stopMusic()` transport. Object URLs exist only for that asynchronous
 * load; the bridge owns the decoded bytes after `playRol()` resolves.
 */
export class RolMusicController {
    constructor(bridge,{edition=0,makeUrl=data=>URL.createObjectURL(new Blob([data])),releaseUrl=url=>URL.revokeObjectURL(url)}={}){
        if(!bridge||typeof bridge.playRol!=='function'||typeof bridge.stopMusic!=='function')throw new TypeError('bridge.playRol and bridge.stopMusic are required');
        if(edition!==0&&edition!==2)invalid(`unsupported music edition: ${edition}`);this.profile=SOURCE_MUSIC_PROFILES[edition];
        this.playlists=this.profile.playlists;
        this.bridge=bridge;this.makeUrl=makeUrl;this.releaseUrl=releaseUrl;this.tracks=new Map();this.bankName='';this.bankData=null;this.active=null;this.menu=null;this.generation=0;this.playbackId=null;this.nextMenuTrack=0;
    }
    async importFiles(files){
        const selected=[...files];if(!selected.length||selected.length>MAX_FILES)invalid('select one .BNK and up to 32 .ROL files');
        let total=0;const names=new Set();
        // Validate all browser File metadata before allocating any file buffers.
        for(const file of selected){
            if(!file||typeof file.name!=='string'||typeof file.arrayBuffer!=='function')invalid('expected a user-selected File');
            const name=upper(file.name),limit=name.endsWith('.BNK')?BNK_MAX_BYTES:ROL_MAX_BYTES;
            if(!/\.(ROL|BNK)$/.test(name))invalid('select only .ROL and .BNK files');
            if(names.has(name))invalid(`duplicate music file: ${name}`);names.add(name);
            if(!Number.isSafeInteger(file.size)||file.size<1||file.size>limit)invalid(`${name}: invalid music file size`);
            total+=file.size;if(total>MAX_COLLECTION_BYTES)invalid('music selection exceeds 32 MiB');
        }
        if(selected.filter(x=>upper(x.name).endsWith('.BNK')).length!==1||selected.length<2)invalid('select exactly one .BNK and at least one .ROL file');
        const loaded=[];for(const file of selected)loaded.push(await readFile(file));
        const banks=loaded.filter(x=>x.name.endsWith('.BNK')),rols=loaded.filter(x=>x.name.endsWith('.ROL'));
        if(banks.length!==1)invalid('select exactly one .BNK file');if(!rols.length)invalid('select at least one .ROL file');
        const bank=banks[0];bnkHeader(bank.data,bank.name);
        const tracks=new Map();let events=0;for(const rol of rols){rolHeader(rol.data,rol.name);if(tracks.has(rol.name))invalid(`duplicate ROL file: ${rol.name}`);try{const player=new RolMusic(rol.data,bank.data);events+=player.song.eventCount;if(events>MAX_COLLECTION_EVENTS)invalid('music selection exceeds 100,000 events');tracks.set(rol.name,{data:rol.data,player});}catch(error){invalid(`${rol.name}: ${error.message}`);}}
        this.stop();this.nextMenuTrack=0;this.tracks=tracks;this.bankName=bank.name;this.bankData=bank.data;return this.catalog();
    }
    catalog(){return [...this.tracks.keys()].sort().map(name=>({name,defaultFor:Object.entries(this.playlists).filter(([,list])=>list.includes(name)).map(([scope])=>scope)}));}
    availableTracks(scope){const names=this.playlists[scope];if(!names)invalid(`unknown source playlist: ${scope}`);return names.filter(name=>this.tracks.has(name));}
    defaultTrack(scope){return this.availableTracks(scope)[0]??null;}
    async play(name,{loop=false}={}){this.menu=null;return this.startTrack(name,loop);}
    async startTrack(name,loop=false){
        const track=this.tracks.get(upper(name));if(!track)invalid(`ROL file is not imported: ${name}`);
        const generation=++this.generation;
        // Retire the previous playback before loading its replacement. Its late
        // completion must not schedule another song while the new one loads.
        this.bridge.stopMusic();this.active=null;this.playbackId=null;
        let rolUrl,bankUrl,started;
        try{
            rolUrl=this.makeUrl(track.data);bankUrl=this.makeUrl(this.bankData);
            started=await this.bridge.playRol(rolUrl,bankUrl,{loop});
        }catch(error){if(generation===this.generation)this.stop();throw error;}
        finally{if(rolUrl!==undefined)this.releaseUrl(rolUrl);if(bankUrl!==undefined)this.releaseUrl(bankUrl);}
        if(generation!==this.generation)return null;
        if(started===false){this.stop();return null;}
        this.playbackId=this.bridge.musicPlaybackId??generation;
        this.active={name:upper(name),loop:Boolean(loop)};return {...this.active};
    }
    stop(){++this.generation;this.bridge.stopMusic();this.active=null;this.playbackId=null;this.menu=null;}
    async enterMenu(scope){
        const names=this.availableTracks(scope);this.menu=scope;
        if(!names.length){this.stop();return null;}
        const index=scope==='moag'?this.nextMenuTrack%names.length:0;
        const generation=this.generation+1,result=await this.startTrack(names[index]);
        if(generation!==this.generation)return null;
        // MOAG advances this cursor only after songstart succeeds. Stops and
        // the off/on toggle retain it, which is the source's next-song action.
        if(result&&scope==='moag')this.nextMenuTrack=(index+1)%names.length;
        return result;
    }
    async ended(playbackId){
        if(!this.active||playbackId!==this.playbackId)return null;
        this.active=null;this.playbackId=null;
        // CCTITLE owns its one-shot music phase; only MOAG cycles its list.
        return this.menu==='moag'?this.enterMenu('moag'):null;
    }
    // CORNCOB.BAT starts a fresh MOAG after ordinary flight. Its internal demo
    // launcher returns to the same MOAG and can request cursor preservation.
    leaveMenuForFlight({resetCursor=true}={}){this.stop();if(resetCursor)this.nextMenuTrack=0;}
    async resumeMenu(){return this.active?{...this.active}:this.menu?this.enterMenu(this.menu):null;}
    suspend(){/* AudioContext suspension is bridge-owned; transport position is preserved there. */}
    resume(){/* Do not replay here: a suspended worklet resumes at its exact sample frame. */}
}
