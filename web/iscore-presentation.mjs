import {bossGlyphs} from './boss.mjs';
import {moagPreflightKey,MOAG_NONE} from './moag-menu-keys.mjs';
import {ISCORE_FONT16} from './iscore-font16.mjs';
const ranks=['2nd Lieutenant','1st Lieutenant','Captain','Major','Lt. Colonel','Colonel','General'];
const word=n=>(n<<16)>>16,view=b=>new DataView(b.buffer,b.byteOffset,b.byteLength),u16=(b,p)=>view(b).getUint16(p,true),s16=(b,p)=>view(b).getInt16(p,true),s32=(b,p)=>view(b).getInt32(p,true);
const string=(b,p=0)=>{let end=p;while(end<b.length&&b[end])end++;return String.fromCharCode(...b.subarray(p,end));};
const dec=(v,n=0,left=false,zero=false)=>{const s=String(v);if(left)return s.padEnd(n,' ');if(zero&&s[0]==='-')return '-'+s.slice(1).padStart(n-1,'0');return s.padStart(n,zero?'0':' ');};
// ISCORE main38C1: temporary copies only; no career mutation or file write.
export function prepareIScore(e,{raw,pilot,theater,header}){
    if(!raw||raw.length<130||pilot?.length!==623)throw Error('The original inflight report needs the current sortie and pilot record.');
    const hasTheater=!(pilot[0x23d]&128);if(hasTheater&&(theater?.length!==623||header?.length!==48))throw Error('The selected theater record is missing.');
    const input=raw.slice(0,130);view(input).setUint16(0,(u16(input,0)|0x14)&0xfffe,true);
    const base=e.cc_score_workspace(),mem=new Uint8Array(e.memory.buffer),read=(p,n)=>mem.slice(p,p+n),result=base+1300,scored=base+1450,factors=base+1600;
    mem.set(input,result);
    const accumulate=(record,p)=>{mem.set(record,p);const status=e.cc_score_accumulate_mode(2,p,623,result,130,scored,134,factors);if(status)throw Error(`Original inflight score calculation failed (${status}).`);return read(p,623);};
    const theaterCopy=hasTheater?accumulate(theater,base+624):null,pilotCopy=accumulate(pilot,base);
    const f=view(read(factors,20));
    return {pilot:pilotCopy,theater:theaterCopy,header:hasTheater?header.slice():null,score:f.getInt32(0,true),mobf:f.getInt32(4,true),pmsf:f.getUint32(8,true),msf:f.getUint32(12,true),factorsValid:!!f.getUint32(16,true)};
}
// Literal mode2/3 display4050..51BF and row helpers5317/534D. Each cell is
// CP437 character + DOS text attribute, matching original direct B8000 writes.
export function iscoreCells(report){
    const p=report.theater??report.pilot,globalPilot=report.pilot,cells=new Uint8Array(65536);let cursor=0,color=0x1e,row=2;
    // init_scr's07h clear occurs before display selects blue background.
    for(let i=0;i<4000;i+=2){cells[i]=32;cells[i+1]=7;}
    const at=(x,y)=>{cursor=(((y-1)*160+(x-1)*2)&65535);};
    const put=s=>{for(const c of s){cells[cursor]=c.charCodeAt(0)&255;cells[(cursor+1)&65535]=color;cursor=(cursor+2)&65535;}};
    const ink=v=>{color=0x10|v;};
    const begin=()=>{at(1,row++);ink(14);put('\xba'+' '.repeat(6));};
    const end=()=>{ink(14);put(' '.repeat(7)+'\xba');};
    const blank=(color=7)=>{begin();ink(color);put(' '.repeat(65));end();};
    const title=report.theater?`PRELIMINARY REPORT:  ${string(report.header)} Theater of Operations`:'PRELIMINARY REPORT';
    const left=Math.trunc((80-title.length-4)/2),right=80-left-title.length-4;
    at(1,1);put('\xc9'+'\xcd'.repeat(Math.max(0,left))+' ');ink(14);put(title);ink(14);put(' '+'\xcd'.repeat(Math.max(0,right))+'\xbb');
    blank();begin();ink(14);put('         PILOT:  ');ink(15);
    const rank=ranks[p[0x256]];if(rank===undefined)throw Error('The pilot rank is outside the original ISCORE table.');
    const name=rank+' '+string(p),resurrections=s16(globalPilot,0x1a),resur=resurrections?`, ${resurrections} resurrection${resurrections===1?'':'s'}`:'';
    put(name+resur+' '.repeat(Math.max(0,35-name.length))+' '.repeat(Math.max(0,13-resur.length)));end();at(80,row-1);put('\xba');
    begin();ink(14);
    const time=s32(p,0x8a),hours=Math.trunc(time/3600),remaining=(time-hours*3600)|0,minutes=word(Math.trunc(remaining/60)),seconds=(remaining-word(minutes*60))|0;
    put(`                 Flying time = ${hours}:${dec(minutes,2,false,true)}:${dec(seconds,2,false,true)}`.padEnd(65,' '));end();
    const status=u16(p,0x1c),statusParts=[[0x4000,'MISSION-ABORTED  '],[2,'KILLED  '],[8,'CAPTURED  '],[16,'AT-HOME  '],[256,'RESCUED  '],[64,'CRASHED-NOT-HOME  '],[32,'EJECTED  '],[512,'CRASHED-AT-HOME  '],[4,'LANDED-AT-HOME  '],[128,'COLLISION  ']];
    begin();ink(14);put('        STATUS:  ');ink(15);put(statusParts.filter(([mask])=>status&mask).map(([,s])=>s).join('').padEnd(48,' '));end();at(80,row-1);put('\xba');
    begin();ink(14);put('         SCORE:  ');const score=s32(p,0x9e),best=s32(p,0x124),total=s32(p,0x230),scoreText=score+' ';
    ink(15);put(scoreText);ink(12);put(score===best?'\x03':' ');put(' '.repeat(Math.max(0,10-scoreText.length)));ink(14);put('BEST:  ');ink(15);put(dec(best,7,true)+'    ');ink(14);put('TOTAL:  ');ink(15);put(dec(total,10,true)+' ');end();
    begin();ink(14);put('          PMSF:  ');ink(15);put(dec(report.pmsf,5,true));ink(14);put('  MSF:  ');ink(15);put(dec(report.msf,5,true));ink(14);put('  MOBF:  ');ink(15);put(dec(report.mobf,9,true)+' '.repeat(12));end();
    blank();begin();ink(12);put('                          This      Career      Best       Career');end();
    begin();ink(14);put(' PRIMARY KILLS:');ink(12);put('          Mission     High      Mission      Total');end();
    const sum=(start,extra=null)=>{let v=0;for(let i=0;i<8;i++){v=word(v+s16(p,start+2*i));if(extra!==null)v=word(v+s16(p,extra+2*i));}return v;};
    const single=offset=>[s16(p,0x1c+offset),s16(p,0x128+offset),s16(p,0xa2+offset),s16(p,0x1ae+offset)];
    const group=(offset,extra=null)=>[0x1c,0x128,0xa2,0x1ae].map(base=>sum(base+offset,extra===null?null:base+extra));
    const stat=(label,values,loss=false)=>{
        begin();ink(12);put(label.padStart(21,' ')+':  ');ink(15);put(dec(values[0],5)+' ');ink(12);put(!loss&&values[0]===values[1]&&values[0]!==0?'\x03':' ');put('    ');
        ink(!loss&&values[0]===values[1]?15:7);put(dec(values[1],5)+'      ');
        if(!loss)ink(values[0]===values[2]?15:7);put(dec(values[2],5)+'      ');ink(7);put(dec(values[3],8));end();
    };
    stat('Mission Objective',single(0x48));stat('TRFRU/Fuel Dumps',single(6));stat('AAA Batteries',group(0x18));stat('Other Sites',group(0x38,0x72));
    begin();ink(14);put('SECONDARY KILLS:                                                 ');end();
    stat('Ground Transports',group(0x28));stat('Enemy Missiles',group(8));blank(14);
    stat('Number of Planes Lost',single(4),true);stat('Friendly Sites Lost',single(0x4a),true);stat('Total Damage',single(0x4c),true);
    blank();blank();blank();at(1,row);ink(14);put('\xc8'+'\xcd'.repeat(78)+'\xbc');
    // main39A3: discard queued DOS keys, put prompt at x25,y24, then getkey.
    at(25,24);put('Type almost any key to continue...');
    return cells.slice(0,4000);
}
const colors=['#000000','#0000aa','#00aa00','#00aaaa','#aa0000','#aa00aa','#aa5500','#aaaaaa','#555555','#5555ff','#55ff55','#55ffff','#ff5555','#ff55ff','#ffff55','#ffffff'];
// VGA mode3 uses9x16 text cells. The ninth column repeats the eighth only
// for the original line-graphics range C0h..DFh; all other glyphs use background.
export function iscorePixels(cells,font=ISCORE_FONT16){
    const rgba=new Uint8ClampedArray(720*400*4),palette=colors.map(c=>[1,3,5].map(p=>parseInt(c.slice(p,p+2),16)));
    for(let cell=0;cell<2000;cell++){
        const ch=cells[cell*2],attr=cells[cell*2+1],fg=palette[attr&15],bg=palette[attr>>4&7],cx=cell%80*9,cy=Math.floor(cell/80)*16;
        for(let y=0;y<16;y++)for(let x=0;x<9;x++){
            const ink=x<8?(font[ch*16+y]&(128>>x)):(ch>=0xc0&&ch<=0xdf&&(font[ch*16+y]&1)),rgb=ink?fg:bg,p=((cy+y)*720+cx+x)*4;
            rgba[p]=rgb[0];rgba[p+1]=rgb[1];rgba[p+2]=rgb[2];rgba[p+3]=255;
        }
    }
    return rgba;
}
export function iscoreCanvas(cells){
    const canvas=document.createElement('canvas');canvas.width=720;canvas.height=400;canvas.className='iscore-screen';canvas.style.cssText='display:block;width:100%;height:auto;aspect-ratio:4/3;image-rendering:pixelated;background:#000';
    const lines=Array.from({length:25},(_,row)=>bossGlyphs(cells.subarray(row*160,row*160+160).filter((_,i)=>!(i&1))));
    canvas.setAttribute('role','img');canvas.setAttribute('aria-label',lines.join('\n'));
    const context=canvas.getContext('2d');if(!context)return iscoreTextElement(cells);
    const image=context.createImageData(720,400);image.data.set(iscorePixels(cells));context.putImageData(image,0,0);return canvas;
}
export function iscoreTextElement(cells){
    const pre=document.createElement('pre');pre.className='iscore-screen';pre.setAttribute('aria-label','Original preliminary sortie report');
    pre.style.cssText='margin:0;background:#000;color:#aaa;white-space:pre;overflow:auto;font:14px/1.15 ui-monospace,monospace;width:max-content;max-width:100%;';
    for(let row=0;row<25;row++){
        let part=[],attribute=-1;
        const flush=()=>{if(!part.length)return;const span=document.createElement('span');span.textContent=bossGlyphs(Uint8Array.from(part));span.style.color=colors[attribute&15];span.style.backgroundColor=colors[(attribute>>4)&7];pre.append(span);part=[];};
        for(let x=0;x<80;x++){const p=(row*80+x)*2;if(cells[p+1]!==attribute){flush();attribute=cells[p+1];}part.push(cells[p]);}flush();if(row<24)pre.append(document.createTextNode('\n'));
    }
    return pre;
}
export function inflightIScore(menu,raw=menu.game.session?.modalResults){
    const career=menu.game.career,theater=career.theater(),report=prepareIScore(menu.game.e,{raw,pilot:career.current(),theater:theater?.pilot,header:theater?.header});
    const box=menu.screen('Preliminary report');box.append(iscoreCanvas(iscoreCells(report)));menu.preflightKeys=true;
    let completed=false;const close=()=>{if(completed)return;completed=true;return menu.actions.resumeMenu();};
    menu.nav([['Return to flight',close]]);
    // Installed MOAG reader shares ISCORE's exact DOS extended-key table:
    // F1 opens/restores BOSS, unsupported extended keys beep and keep waiting.
    menu.keyHandler=event=>{const key=event.moagKey??moagPreflightKey(event);if(key===MOAG_NONE||key<0)return;event.preventDefault();void menu.run(close);};
    return report;
}
