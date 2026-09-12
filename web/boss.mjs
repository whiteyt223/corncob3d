import {decodeBytes,encodeBytes} from './characters.mjs';
export const BOSS_RECORD='BOSS.TXT',MAX_BOSS_BYTES=8*1024*1024;
export const DEFAULT_BOSS_BYTES=Uint8Array.from('\r\n'.repeat(23)+'C:\\>$\r\n\r\n',c=>c.charCodeAt(0));
// TOWER's fopen requests512-byte text buffers. Its read22FF drops CR and
// stops at Ctrl-Z, with a literal extra raw byte when CR ends a DOS read.
function textStream(raw){
  let offset=0,ended=false,pending=[],at=0,extra=0;
  const refill=()=>{
    pending=[];at=0;
    while(!pending.length&&!ended){
      const part=raw.subarray(offset,offset+512);offset+=part.length;
      if(!part.length){ended=true;break;}
      for(let i=0;i<part.length;i++){
        const b=part[i];if(b===26){ended=true;break;}
        if(b!==13)pending.push(b);
        else if(i===part.length-1){if(offset<raw.length)extra=raw[offset++];pending.push(extra);}
      }
    }
  };
  return ()=>{if(at===pending.length)refill();return at<pending.length?pending[at++]:null;};
}
export function parseBoss(bytes=DEFAULT_BOSS_BYTES){
  const raw=bytes instanceof Uint8Array?bytes:new Uint8Array(bytes);
  if(raw.length>MAX_BOSS_BYTES)throw Error('BOSS.TXT is too large.');
  const read=textStream(raw),cells=new Uint8Array(2000).fill(32);let cursorRow=0,cursorColumn=0;
  for(let row=0;row<25;row++){
    const input=[];for(let i=0;i<127;i++){const b=read();if(b===null)break;input.push(b);if(b===10)break;}
    if(!input.length)break;
    let length=input.indexOf(0);if(length<0)length=input.length;
    // Original uses the original strlen value for both trailing-byte checks.
    if(length&&[10,13].includes(input[length-1]))input[length-1]=0;
    if(length>=2&&[10,13].includes(input[length-2]))input[length-2]=0;
    const output=[];
    for(let i=0;i<length&&input[i];i++){
      let b=input[i];if(b===36){if(input[i+1]===36)i++;else{cursorColumn=i;cursorRow=row;b=32;}}
      output.push(b);
    }
    cells.set(output.slice(0,80),row*80);
  }
  if(cursorColumn>=80||cursorRow>=25){cursorColumn=0;cursorRow=0;}
  return {cells,cursor:[cursorRow,cursorColumn]};
}
const CONTROL_GLYPHS='\u0000☺☻♥♦♣♠•◘○◙♂♀♪♫☼►◄↕‼¶§▬↨↑↓→←∟↔▲▼';
export function bossGlyphs(cells){return [...cells].map(b=>b<32?CONTROL_GLYPHS[b]:b===127?'⌂':decodeBytes(Uint8Array.of(b))).join('');}
export function bossScreen(bytes){
  const {cells,cursor:[row,column]}=parseBoss(bytes),pre=document.createElement('pre');pre.className='boss-screen';pre.setAttribute('aria-label','Boss screen');
  const lines=Array.from({length:25},(_,i)=>bossGlyphs(cells.subarray(i*80,i*80+80))),text=lines.join('\n'),index=row*81+column;
  const cursor=document.createElement('span');cursor.className='boss-cursor';cursor.textContent=text[index];
  pre.append(document.createTextNode(text.slice(0,index)),cursor,document.createTextNode(text.slice(index+1)));return pre;
}
function download(bytes){const url=URL.createObjectURL(new Blob([bytes],{type:'application/octet-stream'})),a=document.createElement('a');a.href=url;a.download=BOSS_RECORD;a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);}
export function bossOptions(menu){
  const career=menu.game.career,original=career.records.get(BOSS_RECORD)??DEFAULT_BOSS_BYTES,box=menu.screen('Boss screen');
  const description=document.createElement('p');description.textContent='Customize the screen shown by F1. Use $ to position the cursor; $$ prints a dollar sign. The original display shows up to 25 rows and 80 columns.';
  const privacy=document.createElement('p');privacy.className='muted';privacy.textContent='Import a local text file (up to 8 MiB). Selecting a file immediately replaces your saved boss screen in this browser. It is not uploaded to the website. Boss-screen text is included in exported game saves.';
  const editor=document.createElement('textarea');editor.rows=12;editor.className='boss-editor';editor.value=decodeBytes(original).replace(/\r\n/g,'\n');editor.setAttribute('aria-label','Boss screen text');
  const file=document.createElement('input');file.type='file';file.accept='.txt';file.setAttribute('aria-label','Import BOSS.TXT');
  file.addEventListener('change',()=>{const selected=file.files[0];file.value='';if(selected)void menu.run(async()=>{if(selected.size>MAX_BOSS_BYTES)throw Error('BOSS.TXT is too large.');await career.setBoss(new Uint8Array(await selected.arrayBuffer()));bossOptions(menu);});});
  box.append(description,privacy,editor,file);
  const edited=()=>{if(editor.value.length>MAX_BOSS_BYTES)throw Error('BOSS.TXT is too large.');const text=editor.value.replace(/\r?\n/g,'\r\n');if(text.length>MAX_BOSS_BYTES)throw Error('BOSS.TXT is too large.');return encodeBytes(text);};
  menu.nav([['Save text',async()=>{await career.setBoss(edited());bossOptions(menu);menu.message.textContent='Boss screen saved.';}],['Preview',()=>{const preview=box.querySelector('.boss-screen');preview?.remove();box.append(bossScreen(edited()));}],['Export BOSS.TXT',()=>download(career.records.get(BOSS_RECORD)??DEFAULT_BOSS_BYTES)],['Restore original',async()=>{await career.setBoss(null);bossOptions(menu);}],['Controls',()=>menu.controls()]]);
}
