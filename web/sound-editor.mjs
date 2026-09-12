/*
 * Other Worlds built-in Sounds Editor adapter.
 *
 * MOAG.EXE DD24 loads exactly twelve 3.ADL tables.  Each table is a sequence
 * of hexadecimal `register,value` lines terminated by `ff`; ADL.ASM readasound
 * accepts at most fourteen register pairs.  The controller keeps that file
 * format byte-for-byte representable. The core staging buffer is only the
 * browser adapter for the editor's current twelve tables.
 */
import {decodeBytes} from './characters.mjs';
import {moagPreflightKey,MOAG_NONE} from './moag-menu-keys.mjs';
export const SOUND_ADL_RECORD='3.adl',TABLE_COUNT=12,MAX_PAIRS=14;
export const SOUND_NAMES=Object.freeze([
 ['Engine','engine noise'],['Guns','guns firing'],['Boom','enemy AAA shell exploding'],['Screech','landing gear hitting pavement'],
 ['Blam','plane hit by enemy AAA or KLA'],['Missile','missiles firing'],['Crash','crashlanding'],['Wind','wind sound heard when out of plane'],
 ['Stall','stall indicator sound'],['Saucer',"enemy saucer's non-newtonian drive"],['Orb',"enemy headquarters' nausea-field"],['Kla','enemy KLA missle']
]);
const hex=v=>v.toString(16);
const copy=table=>table.map(([r,v])=>[r,v]);
const invalid=(line,message)=>{throw Error(`3.ADL line ${line}: ${message}`);};
// MOAG D48D calls its C library's `%x,%x` scanner and only tests for two
// conversions. Keep the stored values in the original 16-bit unsigned range;
// text after the second conversion is intentionally ignored by the native code.
const scanHex=(text,start)=>{let at=start;while(text[at]===' '||text[at]==='\t')at++;let sign=1;if(text[at]==='+'||text[at]==='-'){if(text[at++]==='-')sign=-1;}if(text.slice(at,at+2).toLowerCase()==='0x')at+=2;const first=at;while(/[0-9a-f]/i.test(text[at]??''))at++;if(at===first)return null;let value=BigInt(`0x${text.slice(first,at)}`);if(sign<0)value=-value;return [Number(value&0xffffn),at];};
const scanPair=text=>{const left=scanHex(text,0);if(!left||text[left[1]]!==',')return null;const right=scanHex(text,left[1]+1);return right?[left[0],right[0]]:null;};
/** Parse MOAG D405's 16-byte-line `%x,%x` editor file contract. */
export function parseAdl(bytes){
 const text=decodeBytes(bytes instanceof Uint8Array?bytes:new Uint8Array(bytes));
 if(text.includes('\0'))throw Error('3.ADL contains a NUL byte');
 const tables=[];let current=[];
 for(const [index,source] of text.replace(/\r/g,'').split('\n').entries()){
  const raw=source.replace(/^[ \t]*/,'');if(!raw||raw.startsWith('#'))continue;const line=index+1;if(source.length>=16)invalid(line,'MOAG D405 accepts at most fifteen characters per line');
  if(/^ff$/i.test(raw)){if(!current.length)invalid(line,'empty sound table');tables.push(current);current=[];continue;}
  const pair=scanPair(raw);if(!pair)invalid(line,'expected two hexadecimal values separated by a comma or ff');
  if(current.length===MAX_PAIRS)invalid(line,'more than fourteen register pairs');if(pair[0]===0xff)invalid(line,'ff terminates a table and cannot be a register');current.push(pair);
 }
 if(current.length)throw Error('3.ADL ends before ff');if(tables.length!==TABLE_COUNT)throw Error(`3.ADL contains ${tables.length} sound tables; expected 12`);
 return tables;
}
/** Canonical writer matches MOAG DBE7's lowercase `%x,%x\r\n` and `ff\r\n`. */
export function encodeAdl(tables){
 if(!Array.isArray(tables)||tables.length!==TABLE_COUNT)throw Error('expected twelve sound tables');
 const rows=[];for(const table of tables){if(!Array.isArray(table)||!table.length||table.length>MAX_PAIRS)throw Error('invalid sound table length');for(const [register,value] of table){if(!Number.isInteger(register)||!Number.isInteger(value)||register<0||register>254||value<0||value>255)throw Error('invalid OPL register pair');rows.push(`${hex(register)},${hex(value)}`);}rows.push('ff');}
 return Uint8Array.from(rows.join('\r\n')+'\r\n',c=>c.charCodeAt(0));
}
const family=register=>register>=0x20&&register<=0x35?'operator characteristics':register>=0x40&&register<=0x55?'total level / low-pass filter':register>=0x60&&register<=0x75?'attack / decay':register>=0x80&&register<=0x95?'sustain / release':register>=0xe0&&register<=0xf5?'waveform select':register>=0xa0&&register<=0xa8?'voice frequency':register>=0xb0&&register<=0xb8?'voice key/block':register>=0xc0&&register<=0xc8?'connection / feedback':'raw OPL register';
/** Exact bit fields named by MOAG's editor strings. */
export function parameterRows(table){return table.map(([register,value],index)=>({index,register,value,hexRegister:hex(register),family:family(register),fields:
 register>=0x20&&register<=0x35?[['Tremolo',0x80,7],['Vibrato',0x40,6],['Sustain',0x20,5],['Envelope Rate',0x10,4],['Frequency Multiplier',0x0f,0]]:
 register>=0x40&&register<=0x55?[['Low-Pass Filter',0xc0,6],['Total Level',0x3f,0]]:
 register>=0x60&&register<=0x75?[['Attack Rate',0xf0,4],['Decay Rate',0x0f,0]]:
 register>=0x80&&register<=0x95?[['Sustain Level',0xf0,4],['Release Rate',0x0f,0]]:
 register>=0xe0&&register<=0xf5?[['Waveform Select',0x03,0]]:
 register>=0xc0&&register<=0xc8?[['Connection',0x01,0],['Feedback',0x0e,1]]:[]}));}
export function setField(table,rowIndex,mask,shift,next){const tableCopy=copy(table),pair=tableCopy[rowIndex];if(!pair)throw Error('unknown OPL register row');if(!Number.isInteger(next)||next<0||next>(mask>>>shift))throw Error('value is outside the original register field');pair[1]=(pair[1]&~mask)|((next<<shift)&mask);return tableCopy;}
export class SoundEditorController {
 constructor(core,{defaults,initial=defaults,read=async()=>null,write=async()=>{},remove=async()=>{}}={}){if(!core||typeof core.cc_audio_editor_buffer!=='function'||typeof core.cc_audio_editor_commit!=='function')throw Error('source audio editor exports are unavailable');this.core=core;this.defaultBytes=new Uint8Array(defaults);this.defaults=parseAdl(this.defaultBytes);this.initialBytes=new Uint8Array(initial);this.initial=parseAdl(this.initialBytes);this.read=read;this.write=write;this.remove=remove;this.tables=this.initial.map(copy);this.dirty=false;}
 async load(){const saved=await this.read(SOUND_ADL_RECORD);this.tables=saved?parseAdl(saved):this.initial.map(copy);this.dirty=false;this.install();return this.tables.map(copy);}
 install(){this.core.cc_audio_editor_reset();const p=this.core.cc_audio_editor_buffer()>>>0,memory=new Uint8Array(this.core.memory.buffer,p,TABLE_COUNT*(MAX_PAIRS*2+1));for(let index=0;index<TABLE_COUNT;index++){const table=this.tables[index],at=index*(MAX_PAIRS*2+1);for(let i=0;i<table.length;i++){memory[at+i*2]=table[i][0];memory[at+i*2+1]=table[i][1];}memory[at+table.length*2]=0xff;if(this.core.cc_audio_editor_commit(index,table.length))throw Error('core rejected valid 3.ADL table');}}
 table(index){if(!Number.isInteger(index)||index<0||index>=TABLE_COUNT)throw Error('unknown sound');return copy(this.tables[index]);}
 replace(index,table){encodeAdl([...this.tables.slice(0,index),table,...this.tables.slice(index+1)]);const next=copy(table),current=this.tables[index];if(current.length===next.length&&current.every(([r,v],i)=>r===next[i][0]&&v===next[i][1]))return;this.tables[index]=next;this.dirty=true;this.install();}
 setField(index,row,mask,shift,value){this.replace(index,setField(this.table(index),row,mask,shift,value));}
 async restoreDefaults(){// MOAG DADB copies DEF3.ADL to 3.ADL, then clears DS:3056. It does not reload DS:2f78.
  await this.write(SOUND_ADL_RECORD,this.defaultBytes.slice());this.dirty=false;}
 async save(){const bytes=encodeAdl(this.tables);await this.write(SOUND_ADL_RECORD,bytes);this.dirty=false;return bytes;}
 async abandon({save=false}={}){if(save&&this.dirty)await this.save();else await this.load();}
 async export(){return encodeAdl(this.tables);}
 async import(bytes){this.tables=parseAdl(bytes);this.dirty=true;this.install();return this.tables.map(copy);}
}
const el=(tag,text,attrs={})=>{const node=document.createElement(tag);if(text!==undefined)node.textContent=text;Object.assign(node,attrs);return node;};
const downloadAdl=async editor=>{const url=URL.createObjectURL(new Blob([await editor.export()],{type:'text/plain'})),a=el('a');a.href=url;a.download='3.ADL';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);};
/** Browser menu surface mirrors MOAG Change/Restore/Save/Abort. */
export function soundEditorOptions(menu,editor){
 let selected=0;
 // DD24's explicit “Abort and Return to Main” menu choice branches directly
 // to the return path. Escape and Backspace instead reach DC7F: S/s saves;
 // every other answer discards the staged records.
 const abort=async()=>{await editor.abandon({save:false});menu.hangar();};
 const promptAbort=async()=>{if(!editor.dirty)return abort();menu.screen('Save Sound Changes?');menu.preflightKeys=true;menu.content.append(el('p','Save changed sounds before returning to the main menu?'));const save=async()=>{await editor.save();menu.hangar();};menu.nav([['Save & Return',save],['Abort without saving',abort]]);menu.keyHandler=event=>{if(event.repeat)return;const key=event.moagKey??moagPreflightKey(event);if(key===MOAG_NONE)return;event.preventDefault();void menu.run(key===83||key===115?save:abort);};};
 const fieldInput=(row,[label,mask,shift])=>{const limit=mask>>shift,current=(row.value&mask)>>shift,input=el('input',undefined,{type:'number',min:'0',max:String(limit),step:'1',value:String(current),'aria-label':`${label} register ${row.hexRegister}`});input.addEventListener('change',()=>menu.run(async()=>{const value=Number(input.value);editor.setField(selected,row.index,mask,shift,value);render();}));const labelNode=el('label',`${label} `);labelNode.append(input);return labelNode;};
 const render=()=>{const [name,detail]=SOUND_NAMES[selected],box=menu.screen('Sounds Editor');menu.preflightKeys=true;box.append(el('p',`${name} — ${detail}`),el('p','These controls are the original named OPL bit fields.'));
 const choose=el('select');SOUND_NAMES.forEach(([label],i)=>choose.append(el('option',label,{value:String(i),selected:i===selected})));choose.addEventListener('change',()=>{selected=Number(choose.value);render();});box.append(choose);
 const transfer=el('input',undefined,{type:'file',accept:'.adl,text/plain','aria-label':'Import 3.ADL'});transfer.addEventListener('change',()=>menu.run(async()=>{const file=transfer.files?.[0];transfer.value='';if(!file)return;await editor.import(new Uint8Array(await file.arrayBuffer()));render();}));box.append(el('p','Import an original 3.ADL file or export the current twelve tables.'),transfer);
 const table=el('table'),head=el('tr');for(const label of ['Register','Original fields'])head.append(el('th',label));table.append(head);
 for(const row of parameterRows(editor.table(selected))){const tr=el('tr');tr.append(el('td',`$${row.hexRegister}`));const cell=el('td');if(row.fields.length)for(const field of row.fields)cell.append(fieldInput(row,field));else cell.append(el('span','Fixed by this sound table'));tr.append(cell);table.append(tr);}box.append(table);
 menu.nav([['Export 3.ADL',()=>downloadAdl(editor)],['Restore Default Sounds',async()=>{await editor.restoreDefaults();render();}],['Save & Return to Main',async()=>{await editor.save();menu.hangar();}],['Abort and Return to Main',abort],['Operations',abort]]);const exitKeys=event=>{if(event.repeat)return;const key=event.moagKey??moagPreflightKey(event);if(key!==27&&key!==8)return;event.preventDefault();void menu.run(promptAbort);};exitKeys.inputKeys=new Set(['Escape','Backspace']);menu.keyHandler=exitKeys;};render();
}
