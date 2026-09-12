import {BOSS_RECORD,MAX_BOSS_BYTES} from './boss.mjs';
const SOUND_ADL_RECORD='3.adl';
// Original byte records and native-verified MOAG decisions, with browser IO.
const PILOT=623,META=28,now=()=>Math.floor(Date.now()/1000)>>>0;
const view=b=>new DataView(b.buffer,b.byteOffset,b.byteLength);
const u16=(b,p)=>view(b).getUint16(p,true),write16=(b,p,n)=>view(b).setUint16(p,n,true);
export class Career{
    constructor(e,storage,initialFile,definitions,encodeName){this.e=e;this.storage=storage;this.initialFile=initialFile;this.definitions=definitions;this.encodeName=encodeName;this.records=new Map();this.transientTheaters=new Set();this.trainee=new Uint8Array(PILOT);this.metadata=new Uint8Array(META);this.pilots=[];this.backing=new Uint8Array(16*PILOT);this.active=-1;}
    put(offset,bytes){const p=this.e.cc_browser_workspace()+offset;new Uint8Array(this.e.memory.buffer,p,bytes.length).set(bytes);return p;}
    get(pointer,length){return new Uint8Array(this.e.memory.buffer,pointer,length).slice();}
    callRecord(name,record,...args){const p=this.put(0,record),status=this.e[name](p,...args);if(status<0)throw Error(`Original career operation failed (${status}).`);return {status,record:this.get(p,PILOT)};}
    decode(file){const out=this.e.cc_browser_workspace(),p=this.put(30000,file),count=this.e.cc_career_decode_mode(this.e.cc_edition_is_other_worlds(),p,file.length,out,out+64,15*PILOT);if(count<0)throw Error('Invalid pilot file.');return {metadata:this.get(out,META),pilots:Array.from({length:count},(_,i)=>this.get(out+64+i*PILOT,PILOT)),active:this.e.cc_career_active(out+64,count)};}
    async load(){this.transientTheaters.clear();this.records=this.storage?await this.storage.records():new Map();const decoded=this.decode(this.records.get('pilot.scr')??this.initialFile);Object.assign(this,decoded);this.backing.fill(0);this.pilots.forEach((p,i)=>this.backing.set(p,i*PILOT));const p=this.e.cc_browser_workspace(),name=this.encodeName('trainee'),n=this.put(12000,name);if(this.e.cc_career_new(p,n,name.length,1)<0)throw Error('Could not initialize trainee.');this.trainee=this.get(p,PILOT);}
    current(){return this.active<0?this.trainee:(this.pilots[this.active]??this.backing.slice(this.active*PILOT,(this.active+1)*PILOT));}
    setCurrent(record){if(this.active<0)this.trainee=record;else if(this.active<this.pilots.length)this.pilots[this.active]=record;else this.backing.set(record,this.active*PILOT);}
    encodeRoster(){const metadata=this.metadata.slice(),time=now();view(metadata).setUint16(0,time&65535,true);view(metadata).setUint32(16,time,true);const m=this.put(0,metadata),roster=new Uint8Array(this.pilots.length*PILOT);this.pilots.forEach((p,i)=>roster.set(p,i*PILOT));const p=this.put(64,roster),out=this.e.cc_browser_workspace()+12000,n=this.e.cc_career_encode(m,p,this.pilots.length,out,10000);if(n<0)throw Error('Could not encode pilot file.');return this.get(out,n);}
    async _setControls(kind,force){
        const p=this.e.cc_browser_workspace(),words=new Uint16Array(this.e.memory.buffer,p,3);
        words.set([u16(this.metadata,2),force,u16(this.metadata,4)]);this.e.cc_menu_controls_choice(p,kind);
        const result=[...words];write16(this.metadata,2,result[0]);write16(this.metadata,4,result[2]);await this.save();return result[1];
    }
    async _setSoundAdl(bytes){
        const file=new Uint8Array(bytes);if(!file.length||file.length>2048)throw Error('Invalid 3.ADL size.');this.records.set(SOUND_ADL_RECORD,file);await this.save();
    }
    async _setBoss(bytes){
        if(bytes===null)this.records.delete(BOSS_RECORD);
        else{const file=new Uint8Array(bytes);if(file.length>MAX_BOSS_BYTES)throw Error('BOSS.TXT is too large.');this.records.set(BOSS_RECORD,file);}
        await this.save();
    }
    async save(){if(!this.storage)return;const named=new Map([...this.records].filter(([name])=>name===BOSS_RECORD||name===SOUND_ADL_RECORD||name==='pilot.hsc'||name==='3d.cfg'||(name.startsWith('THT/')&&!this.transientTheaters.has(name))));named.set('pilot.scr',this.encodeRoster());await this.storage.commit(named,{replace:true});this.records.set('pilot.scr',named.get('pilot.scr'));}
    async _create(name){const encoded=this.encodeName(name),joined=new Uint8Array(this.pilots.length*PILOT);this.pilots.forEach((p,i)=>joined.set(p,i*PILOT));const p=this.put(0,joined),n=this.put(12000,encoded),gate=this.e.cc_career_name_gate(p,this.pilots.length,n,encoded.length);if(gate<0)throw Error(gate===-3?'That pilot name is already in use.':gate===-4?'The duty roster is full.':'Use 1–20 characters from the original character set.');const out=this.e.cc_browser_workspace()+14000;if(this.e.cc_career_new(out,n,encoded.length,0)<0)throw Error('Could not create pilot.');this.pilots.push(this.get(out,PILOT));await this._activate(this.pilots.length-1);}
    selectionGate(index){if(index<0)return 0;return this.callRecord('cc_career_select_gate',this.pilots[index],now()).status;}
    async _activate(index,{resurrect=false}={}){
        if(index< -1||index>=this.pilots.length)throw Error('Unknown pilot.');
        if(index>=0){const gate=this.selectionGate(index);if(gate===1)throw Error('This pilot is unavailable for duty.');if(gate===2){if(!resurrect)return {resurrection:true};this.pilots[index]=this.callRecord('cc_career_resurrect',this.pilots[index],now(),0).record;}}
        // Original deletion can leave the active pointer on a stale backing
        // slot. Activation changes only the old/new flag words; its count is
        // a bounds contract, so include that slot without adding a live pilot.
        const joined=this.rosterBacking(),span=Math.max(this.pilots.length,this.active+1),p=this.put(0,joined),t=this.put(12000,this.trainee);
        if(this.e.cc_career_activate(p,span,t,this.active,index)<0)throw Error('Could not activate pilot.');
        this.unpackRoster(this.get(p,16*PILOT));this.trainee=this.get(t,PILOT);this.active=index;await this.save();return {resurrection:false};
    }
    rosterBacking(){const b=this.backing.slice();this.pilots.forEach((p,i)=>b.set(p,i*PILOT));return b;}
    unpackRoster(b,count=this.pilots.length){this.backing=b;this.pilots=Array.from({length:count},(_,i)=>b.slice(i*PILOT,(i+1)*PILOT));}
    async _rename(index,name){
        const bytes=this.encodeName(name),p=this.put(0,this.rosterBacking()),n=this.put(12000,bytes),scores=this.records.get('pilot.hsc'),s=scores?this.put(25000,scores):0;
        const result=this.e.cc_career_rename_pilot(p,this.pilots.length,index,n,bytes.length,s);if(result<0)throw Error(result===-3?'That pilot name is already in use.':'Could not rename pilot.');
        this.unpackRoster(this.get(p,16*PILOT));if(scores)this.records.set('pilot.hsc',this.get(s,5040));await this.save();
    }
    async _deletePilot(index){
        const p=this.put(0,this.rosterBacking()),state=this.e.cc_browser_workspace()+12000,scores=this.records.get('pilot.hsc'),score=scores?this.put(25000,scores):0,ids=state+16;
        const v=new DataView(this.e.memory.buffer);v.setUint16(state,this.pilots.length,true);v.setInt16(state+2,this.active,true);
        const n=this.e.cc_career_delete_pilot(p,state,index,score,ids);if(n<0)throw Error('Could not delete pilot.');
        for(const id of this.get(ids,n)){this.records.delete(`THT/${id}.THT`);this.transientTheaters.delete(`THT/${id}.THT`);}
        this.unpackRoster(this.get(p,16*PILOT),v.getUint16(state,true));this.active=v.getInt16(state+2,true);if(scores)this.records.set('pilot.hsc',this.get(score,5040));await this.save();
    }
    async _deleteTheater(index){const r=this.callRecord('cc_career_close_theater',this.current(),index);this.setCurrent(r.record);this.records.delete(`THT/${r.status}.THT`);this.transientTheaters.delete(`THT/${r.status}.THT`);await this.save();}
    async _resurrect(index){
        if(index<0||index>=this.pilots.length)throw Error('Choose a named pilot.');
        const result=this.callRecord('cc_career_resurrect',this.pilots[index],now(),1);if(result.status)throw Error(result.status===1?'This pilot is still in the stockade.':'This pilot does not need resurrection.');this.pilots[index]=result.record;
        const pilot=this.pilots[index];for(let i=0;i<pilot[0x23c];i++){const name=`THT/${pilot[0x23e+i]}.THT`,file=this.records.get(name);if(file){const changed=file.slice(),p=this.put(0,file.subarray(48,671));this.e.cc_career_resurrect_theater(p);changed.set(this.get(p,PILOT),48);this.records.set(name,changed);}}
        await this.save();
    }
    openTheaters(){const pilot=this.current();return Array.from({length:pilot[0x23c]},(_,index)=>{const id=pilot[0x23e+index],bytes=this.records.get(`THT/${id}.THT`);if(!bytes)throw Error(`Saved theater ${id} is missing.`);return {id,index,bytes,header:bytes.slice(0,48),pilot:bytes.slice(48,671),world:bytes.slice(671)};});}
    theater(){return this.openTheaters().find(t=>t.index===this.current()[0x23d])??null;}
    async _openTheater(definition,world){
        const p=this.put(0,this.current()),d=this.put(1024,definition),h=this.e.cc_browser_workspace()+2048,t=h+64;
        if(this.e.cc_career_new_theater(p,d,h,t)<0)throw Error('Could not open theater.');
        let id=1;while(id<=255&&this.records.has(`THT/${id}.THT`))id++;if(id>255)throw Error('All 255 original theater file slots are in use. Delete an unused campaign before opening another.');
        const file=new Uint8Array(671+world.length);file.set(this.get(h,48));file.set(this.get(t,PILOT),48);file.set(world,671);
        if(this.e.cc_career_link_theater(p,id)<0)throw Error('This pilot has the maximum number of open theaters.');this.e.cc_options_context(p+0x234,1);this.setCurrent(this.get(p,PILOT));this.records.set(`THT/${id}.THT`,file);if(this.active<0)this.transientTheaters.add(`THT/${id}.THT`);const selected=this.theater();if(this.active>=0)await this.save();return selected;
    }
    async _selectTheater(index){const r=this.callRecord('cc_career_select_theater',this.current(),index);const p=this.put(0,r.record);this.e.cc_options_context(p+0x234,1);this.setCurrent(this.get(p,PILOT));const selected=this.theater();if(this.active>=0)await this.save();return selected;}
    async _selectTraining(){const p=this.put(0,this.current());this.e.cc_options_select_training(p);this.setCurrent(this.get(p,PILOT));await this.save();}
    async _setOptions(action,field,value){const p=this.put(0,this.current()),o=p+0x234,e=this.e;let result=0;if(action==='toggle')result=e.cc_options_toggle(o,field);else if(action==='enemy')result=e.cc_options_set_enemy(o,field,value);else if(action==='defaults')e.cc_options_defaults(o);else throw Error('Unknown option.');if(result)throw Error('That option is outside the original range.');e.cc_options_context(o,Number(!(this.current()[0x23d]&128)));this.setCurrent(this.get(p,PILOT));await this.save();}
    score(raw,mode=this.e.cc_edition_is_deluxe()?1:0){const p=this.put(22000,raw),out=this.e.cc_browser_workspace()+24000,status=this.e.cc_score_compute(p,raw.length,mode,out);if(status)throw Error(`Original score calculation failed (${status}).`);const v=view(this.get(out,20));return {score:v.getInt32(0,true),pilotFactor:v.getUint32(8,true),missionFactor:v.getUint32(12,true),factorsValid:!!v.getUint32(16,true)};}
    highScores(){return this.records.get('pilot.hsc')??new Uint8Array(5040);}
    async _clearScores(list){const p=this.put(0,this.highScores());if(this.e.cc_career_scores_clear(p,5040,list)<0)throw Error('Could not clear score list.');this.records.set('pilot.hsc',this.get(p,5040));await this.save();}
    updateScores(pilot,theater){const scores=this.highScores(),p=this.put(0,pilot),s=this.put(25000,scores),label=theater?.header.subarray(0,28)??new Uint8Array(),n=this.put(12000,label),length=label.indexOf(0);if(this.e.cc_career_scores_update(s,5040,p,Number(this.active<0),n,length<0?label.length:length)<0)throw Error('Could not update original high scores.');this.records.set('pilot.hsc',this.get(s,5040));}
    async _finish(raw,world,{engineError=0,errorChoice=0,requestedExtraPlane=0,config}={}){
        const e=this.e,current=this.current(),status=u16(raw,0),gate=e.cc_flow_commit_gate(status,current[0x23b],0,engineError,errorChoice);
        if(gate===2)return {needsErrorChoice:true};
        if(config?.length===52)this.records.set('3d.cfg',config.slice());
        const report={accepted:gate===0,reason:gate,score:this.score(raw),awards:[],promoted:false};
        if(!gate){
            let pilot=current.slice();write16(pilot,0x16,e.cc_flow_career_flags(u16(pilot,0x16),status));
            const accumulate=record=>{const p=this.put(0,record),r=this.put(22000,raw),out=e.cc_browser_workspace()+23000;if(e.cc_score_accumulate_mode(e.cc_edition_is_deluxe()?1:0,p,PILOT,r,raw.length,out,134,0))throw Error('Could not accumulate sortie result.');return this.get(p,PILOT);};
            pilot=accumulate(pilot);const theater=this.theater();let theaterPilot;
            if(theater){theaterPilot=theater.pilot.slice();write16(theaterPilot,0x16,e.cc_flow_career_flags(u16(theaterPilot,0x16),status));theaterPilot=accumulate(theaterPilot);const h=this.put(1024,theater.header);e.cc_flow_theater_totals(h+38,requestedExtraPlane,u16(raw,72));theater.header=this.get(h,48);}
            const penalty=this.callRecord('cc_career_stockade',pilot,now());pilot=penalty.record;report.stockade=penalty.status===1;
            if(!report.stockade){
                const opened=this.openTheaters();if(theater)opened[theater.index]=theater;
                const records=new Uint8Array(opened.length*34);opened.forEach((t,i)=>{records.set(t.header.subarray(0,28),i*34);write16(records,i*34+28,u16(t.header,40));write16(records,i*34+30,u16(t.header,42));write16(records,i*34+32,u16(t.header,46));});
                const definitions=new Uint8Array(this.definitions.length/48*30);for(let i=0;i<this.definitions.length/48;i++){definitions.set(this.definitions.subarray(i*48,i*48+28),i*30);write16(definitions,i*30+28,u16(this.definitions,i*48+44));}
                const p=this.put(0,pilot),t=this.put(25000,records),d=this.put(27000,definitions),out=e.cc_browser_workspace()+29000;
                const result=e.cc_score_progress_mode(e.cc_edition_is_other_worlds(),p,PILOT,t,opened.length,d,this.definitions.length/48,theaterPilot?view(theaterPilot).getInt16(0x1b2,true):0,out);if(result)throw Error(`Original awards evaluation failed (${result}).`);
                pilot=this.get(p,PILOT);const awarded=this.get(out,26);report.awards=Array.from({length:10},(_,i)=>u16(awarded,i*2));report.promoted=!!u16(awarded,20);report.specialPromotion=!!u16(awarded,22);
                const changed=this.get(t,records.length);opened.forEach((t,i)=>{write16(t.header,46,u16(changed,i*34+32));if(!theater||t.id!==theater.id){const original=t.bytes.slice();original.set(t.header);this.records.set(`THT/${t.id}.THT`,original);}});
                if(theater)theater.header=opened[theater.index].header;
            }
            report.rank=pilot[0x256];this.updateScores(pilot,theater);this.setCurrent(pilot);
            if(theater){theaterPilot[0x256]=pilot[0x256];const file=new Uint8Array(671+world.length);file.set(theater.header);file.set(theaterPilot,48);file.set(world,671);this.records.set(`THT/${theater.id}.THT`,file);}
        }
        const p=this.put(0,this.current()),t=this.put(12000,this.trainee),changed=e.cc_career_after_sortie(p,t,Number(this.active<0),now());if(changed<0)throw Error('Could not finish pilot report.');if(this.active<0)this.trainee=this.get(p,PILOT);else{this.setCurrent(this.get(p,PILOT));this.trainee=this.get(t,PILOT);}if(changed)this.active=-1;await this.save();return report;
    }
}

// Mutate a private candidate and publish it only after its original-record
// transaction succeeds. A rejected browser write leaves the active checkpoint
// untouched, so retrying cannot count a sortie twice.
for(const name of ['create','activate','openTheater','selectTheater','finish','rename','deletePilot','deleteTheater','resurrect','clearScores','selectTraining','setOptions','setControls','setBoss','setSoundAdl'])Career.prototype[name]=function(...args){
    const run=async()=>{
        const candidate=Object.assign(Object.create(Career.prototype),this,{pilots:this.pilots.map(p=>p.slice()),backing:this.backing.slice(),trainee:this.trainee.slice(),metadata:this.metadata.slice(),records:new Map([...this.records].map(([k,v])=>[k,v.slice()])),transientTheaters:new Set(this.transientTheaters)});
        const result=await candidate['_'+name](...args);
        for(const key of ['pilots','backing','trainee','metadata','records','transientTheaters','active'])this[key]=candidate[key];
        return result;
    };
    const task=(this.pending??Promise.resolve()).then(run);this.pending=task.catch(()=>{});return task;
};
