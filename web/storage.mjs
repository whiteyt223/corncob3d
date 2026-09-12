import {BOSS_RECORD} from './boss.mjs';
const SOUND_ADL_RECORD='3.adl',MAX_SOUND_ADL_BYTES=2048;
// Browser persistence for original-format game records. Transactions commit a
// named pilot and its accepted theater world together; trainee stays in memory.
const DATABASE='corncob-source-port',STORE='records',MAX_BYTES=8*1024*1024;
const editionName=edition=>{if(edition===0)return 'shareware-3.42';if(edition===2)return 'other-worlds';throw Error('Unsupported save edition.');};
const allowed=name=>name===BOSS_RECORD||name===SOUND_ADL_RECORD||name==='pilot.scr'||name==='pilot.hsc'||name==='3d.cfg'||/^THT\/[0-9]{1,5}\.THT$/.test(name);
const hex=bytes=>Array.from(new Uint8Array(bytes),b=>b.toString(16).padStart(2,'0')).join('');
const hash=async bytes=>hex(await crypto.subtle.digest('SHA-256',bytes));
const bytes=value=>value instanceof Uint8Array?value:new Uint8Array(value);
function base64(value){const b=bytes(value),chunks=[];for(let i=0;i<b.length;i+=8192)chunks.push(String.fromCharCode(...b.subarray(i,i+8192)));return btoa(chunks.join(''));}
function unbase64(value){if(typeof value!=='string'||value.length>MAX_BYTES*2)throw Error('Save record is too large.');const s=atob(value);return Uint8Array.from(s,c=>c.charCodeAt(0));}
function validate(name,value){if(!allowed(name))throw Error('Unknown save record.');if(value.byteLength>MAX_BYTES)throw Error('Save record is too large.');if(name===SOUND_ADL_RECORD&&(!value.byteLength||value.byteLength>MAX_SOUND_ADL_BYTES))throw Error('Invalid 3.ADL size.');if(name==='3d.cfg'&&value.byteLength!==52)throw Error('Invalid configuration size.');if(name==='pilot.hsc'&&value.byteLength!==5040)throw Error('Invalid high-score file size.');if(name==='pilot.scr'&&(value.byteLength<29||value.byteLength>29+623*15))throw Error('Invalid pilot file size.');if(name.startsWith('THT/')&&value.byteLength<683)throw Error('Invalid theater file size.');}
// Only actual IndexedDB write/transaction failures carry this type. Record
// validation errors remain ordinary errors and cannot offer persistence recovery.
export class SaveWriteError extends Error{
    constructor(cause){super(cause?.message??'Saving was interrupted.',{cause});this.name='SaveWriteError';}
}
export class GameStorage{
    constructor(database,edition=0){editionName(edition);this.database=database;this.edition=edition;this.persistent=true;}
    static async open(edition=0){
        editionName(edition);const databaseName=edition===0?DATABASE:`${DATABASE}-other-worlds`;
        if(!globalThis.indexedDB)throw Error('This browser cannot store game saves.');
        const database=await new Promise((resolve,reject)=>{const request=indexedDB.open(databaseName,1);request.onupgradeneeded=()=>request.result.createObjectStore(STORE);request.onsuccess=()=>resolve(request.result);request.onerror=()=>reject(request.error);request.onblocked=()=>reject(Error('Close other Corncob tabs to open game saves.'));});
        return new GameStorage(database,edition);
    }
    async read(name){return new Promise((resolve,reject)=>{const tx=this.database.transaction(STORE,'readonly'),request=tx.objectStore(STORE).get(name);request.onsuccess=()=>resolve(request.result?new Uint8Array(request.result):null);request.onerror=()=>reject(request.error);});}
    async records(){return new Promise((resolve,reject)=>{const result=new Map(),tx=this.database.transaction(STORE,'readonly'),request=tx.objectStore(STORE).openCursor();request.onsuccess=()=>{const cursor=request.result;if(cursor){result.set(cursor.key,new Uint8Array(cursor.value));cursor.continue();}};tx.oncomplete=()=>resolve(result);tx.onerror=()=>reject(tx.error);tx.onabort=()=>reject(tx.error??Error('Reading saves was interrupted.'));});}
    async commit(records,{replace=false}={}){
        const entries=[...records].map(([name,value])=>[name,bytes(value).slice()]);let total=0;for(const [name,value] of entries){validate(name,value);total+=value.length;}if(total>MAX_BYTES)throw Error('Save collection is too large.');
        return new Promise((resolve,reject)=>{const tx=this.database.transaction(STORE,'readwrite'),store=tx.objectStore(STORE);if(replace)store.clear();for(const [name,value] of entries)store.put(value.buffer,name);tx.oncomplete=()=>resolve();tx.onerror=()=>reject(tx.error);tx.onabort=()=>reject(tx.error??Error('Saving was interrupted.'));}).catch(error=>{throw new SaveWriteError(error);});
    }
    async export(){
        const records=[];for(const [name,value] of await this.records())records.push({name,sha256:await hash(value),data:base64(value)});
        return new Blob([JSON.stringify({format:'corncob-source-port-save',version:1,edition:editionName(this.edition),records})],{type:'application/json'});
    }
    async import(file,validateGameRecords){
        if(file.size>MAX_BYTES*2)throw Error('Save file is too large.');
        const archive=JSON.parse(await file.text());if(archive.format!=='corncob-source-port-save'||archive.version!==1||!Array.isArray(archive.records)||archive.records.length>362)throw Error('Unsupported save file.');
        if((archive.edition??'shareware-3.42')!==editionName(this.edition))throw Error('This save belongs to a different Corncob edition. Open that edition to import it.');
        const records=new Map();let total=0;for(const record of archive.records){if(records.has(record.name))throw Error('Duplicate save record.');const value=unbase64(record.data);validate(record.name,value);total+=value.length;if(total>MAX_BYTES)throw Error('Save collection is too large.');if(await hash(value)!==record.sha256)throw Error('Save checksum does not match.');records.set(record.name,value);}
        // Original decoders and cross-record references must validate before IO.
        await validateGameRecords(records);await this.commit(records,{replace:true});return records;
    }
    close(){this.database.close();}
}
// Preserve the same transaction/import/export contract when browser storage is
// unavailable. Named careers remain exportable for the duration of this visit.
export class MemoryGameStorage extends GameStorage{
    constructor(edition=0){super(null,edition);this.persistent=false;this.saved=new Map();}
    async read(name){return this.saved.get(name)?.slice()??null;}
    async records(){return new Map([...this.saved].map(([name,value])=>[name,value.slice()]));}
    async commit(records,{replace=false}={}){
        const candidate=replace?new Map():new Map([...this.saved].map(([name,value])=>[name,value.slice()]));
        for(const [name,value] of records){const data=bytes(value).slice();validate(name,data);candidate.set(name,data);}
        if([...candidate.values()].reduce((sum,value)=>sum+value.length,0)>MAX_BYTES)throw Error('Save collection is too large.');
        this.saved=candidate;
    }
    close(){}
}
