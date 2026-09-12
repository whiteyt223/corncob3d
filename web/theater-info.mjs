import {decodeBytes,encodeBytes} from './characters.mjs';
import {moagPreflightKey,MOAG_NONE,MOAG_BOSS,MOAG_BEEP} from './moag-menu-keys.mjs';
import {parseBoss,bossGlyphs,BOSS_RECORD} from './boss.mjs';

// Actual TU5DCC table: the source places field0 at bottom-right, field8 top-left.
export const TU_FIELD_POSITIONS=Object.freeze([
    [63,65,60],[61,60,60],[63,63,60],[39,40,36],[35,40,36],
    [37,37,36],[6,8,7],[5,6,7],[6,6,7]
].map((x,index)=>Object.freeze({index,firstX:x[0],lastX:x[1],countsX:x[2],y:18-4*(index%3)})));
export const TU_SELECTOR_TITLE='Get Detailed info about Airfield';
export const TU_DETAIL_ROWS=22,TU_DETAIL_COLUMNS=77;
const COLORS=['#000000','#0000aa','#00aa00','#00aaaa','#aa0000','#aa00aa','#aa5500','#aaaaaa','#555555','#5555ff','#55ff55','#55ffff','#ff5555','#ff55ff','#ffff55','#ffffff'];
const bytes=(e,p,n)=>new Uint8Array(e.memory.buffer,p,n);
const signed=n=>n&32768?n-65536:n;
const centered=(text,width=75)=>' '.repeat(Math.max(0,Math.floor((width-text.length)/2)))+text+' '.repeat(Math.max(0,Math.ceil((width-text.length)/2)));
function cstring(e,p){const m=new Uint8Array(e.memory.buffer);let end=p;while(end<m.length&&m[end])end++;return decodeBytes(m.subarray(p,end));}
export function tuSections(raw){
    return decodeBytes(raw).split('\f\r\n').map(part=>{
        const lines=part.split('\r\n');if(lines.at(-1)==='')lines.pop();return lines;
    });
}
export function tuDetailFooter(lines){return lines<=TU_DETAIL_ROWS?'Type <CR> to return...':'Type ↑↓ <P-UP> <P-DN> etc, or <CR> to return...';}

// Owns a child display only. Parent owns source modal entry/return, clocks,
// cockpit restoration and persistence. This adapter never changes those states.
export class TheaterInfo {
    constructor({document,parent,game,session,callbacks={}}){
        if(!document||!parent||!game?.e||!session)throw Error('Theater information needs a document, parent, game and session.');
        Object.assign(this,{document,parent,game,session,callbacks});
        this.active=false;this.selected=4;this.top=0;this.mode='selector';this.busy=false;
    }
    show(){
        if(this.active)throw Error('Theater information is already open.');
        const e=this.game.e,world=this.session.modalWorld;
        if(!(world instanceof Uint8Array)||!world.length)throw Error('The original F6 world snapshot is missing.');
        if(!e.cc_edition_is_other_worlds?.())throw Error('The original TU child requires Other Worlds.');
        const p=e.cc_world_file_buffer();bytes(e,p,world.length).set(world);
        if(e.cc_tu_generate(p,world.length,0))throw Error('Could not generate the original theater information.');
        this.text=bytes(e,e.cc_towers_text(),e.cc_towers_text_size()).slice();
        this.sections=tuSections(this.text);
        const result=new Uint32Array(e.memory.buffer,e.cc_airfield_summary_result(),4),summary=bytes(e,e.cc_airfield_summary_buffer(),126).slice(),view=new DataView(summary.buffer);
        this.remaining=signed(result[0]&65535);this.completed=signed(result[1]&65535);this.single=!!result[2];
        this.fields=TU_FIELD_POSITIONS.map(position=>({...position,
            first:cstring(e,e.cc_towers_field_name(position.index,0)),last:cstring(e,e.cc_towers_field_name(position.index,1)),
            planes:view.getInt16(position.index*14+8,true),objectives:view.getInt16(position.index*14+10,true)}));
        this.selected=4;this.top=0;this.mode='selector';this.active=true;this.parent.hidden=false;
        this.render();this.element.focus();return this;
    }
    async input(key){
        if(!this.active||this.busy||this.boss)return;
        this.busy=true;
        try{
            if(this.mode==='selector'){
                if(key===27||key===8){await this.close(key===8?4:-1);return;}
                if(key===10||key===105){this.mode='details';this.top=0;this.render();return;}
                const result=this.game.e.cc_towers_airfield_key(this.selected,Number(this.single),key)>>>0;
                if(result&0x20000)await this.callbacks.beep?.();
                const selected=result&65535;if(selected!==this.selected){this.selected=selected;this.render();}
            }else{
                const result=this.game.e.cc_tu_view_key(this.top,this.lines.length,key)>>>0;
                if(result&0x20000)await this.callbacks.beep?.();
                if(result&0x10000){this.mode='selector';this.render();}
                else if((result&65535)!==this.top){this.top=result&65535;this.render();}
            }
        }finally{this.busy=false;}
    }
    key(event){
        if(!this.active||event.target?.tagName==='INPUT'||event.target?.tagName==='TEXTAREA'||event.target?.tagName==='SELECT')return false;
        // KeyboardEvent properties are inherited accessors, not enumerable fields.
        const key=event.moagKey??moagPreflightKey({code:event.code??'',key:event.key,shiftKey:event.shiftKey,ctrlKey:event.ctrlKey,altKey:event.altKey,metaKey:event.metaKey});
        if(key===MOAG_NONE)return false;
        event.preventDefault?.();
        if(this.busy)return true;
        // TU44C0 handles F1 before either selector or detail reader.
        if(this.boss){this.restoreBoss();return true;}
        if(key===MOAG_BOSS){this.run(()=>this.showBoss());return true;}
        if(key===MOAG_BEEP){this.run(()=>this.readerBeep());return true;}
        this.run(()=>this.input(key));return true;
    }
    async readerBeep(){if(this.busy)return;this.busy=true;try{await this.callbacks.beep?.();}finally{this.busy=false;}}
    run(action){return Promise.resolve().then(action).catch(error=>{if(this.callbacks.error)this.callbacks.error(error);else if(this.message)this.message.textContent=error.message;});}
    // PreflightBeep queues and replays DOS keys through the active child reader.
    get root(){return this.parent;}
    get lines(){return this.sections[this.selected+1]??[];}
    async close(selectorReturn){
        if(!this.active)return;this.active=false;this.parent.hidden=true;
        await this.callbacks.close?.({status:0,selectorReturn});
    }
    dispose(){this.active=false;this.boss=null;this.owned?.remove();}
    showBoss(){
        if(!this.active||this.boss)return;
        const {cells,cursor:[row,column]}=parseBoss(this.game.career?.records?.get(BOSS_RECORD));
        this.boss={nodes:[...this.owned.childNodes]};this.owned.replaceChildren();
        const pre=this.document.createElement('pre');pre.className='boss-screen';pre.tabIndex=0;pre.setAttribute('aria-label','Boss screen');
        const text=Array.from({length:25},(_,i)=>bossGlyphs(cells.subarray(i*80,i*80+80))).join('\n'),index=row*81+column;
        const cursor=this.document.createElement('span');cursor.className='boss-cursor';cursor.textContent=text[index];
        pre.append(this.document.createTextNode(text.slice(0,index)),cursor,this.document.createTextNode(text.slice(index+1)));
        this.owned.append(pre);this.actions([['Return',()=>this.restoreBoss()]]);pre.focus();
    }
    restoreBoss(){if(!this.boss)return;const {nodes}=this.boss;this.boss=null;this.owned.replaceChildren(...nodes);this.element.focus();}
    // TU3904 stores at B800:160*(y-1)+2*(x-1); public source coordinates are one-based.
    elementAt(tag,text,x,y,color=0x0f){
        const n=this.document.createElement(tag);n.textContent=text;
        n.dataset.sourceX=String(x);n.dataset.sourceY=String(y);n.dataset.sourceColor=color.toString(16);
        Object.assign(n.style,{position:'absolute',left:`${x-1}ch`,top:`${(y-1)*1.25}em`,margin:'0',padding:'0',font:'inherit',lineHeight:'1.25',whiteSpace:'pre',color:COLORS[color&15],backgroundColor:COLORS[color>>4&15]});
        this.element.append(n);return n;
    }
    textRun(text,x,y,color=0x0f){return this.elementAt('span',text,x,y,color);}
    render(){
        const d=this.document;this.parent.replaceChildren();this.owned=d.createElement('div');this.owned.className='theater-info-host';this.parent.append(this.owned);
        const wrapper=d.createElement('div');wrapper.style.overflowX='auto';
        this.element=d.createElement('div');this.element.className='theater-info';this.element.tabIndex=0;this.element.setAttribute('role','dialog');this.element.setAttribute('aria-label',this.mode==='selector'?TU_SELECTOR_TITLE:`Objects near ${this.fields[this.selected].first} ${this.fields[this.selected].last}`);
        Object.assign(this.element.style,{position:'relative',width:'80ch',height:'31.25em',fontFamily:'monospace',fontSize:'14px',lineHeight:'1.25',backgroundColor:COLORS[this.mode==='details'?6:0],color:COLORS[7]});
        wrapper.append(this.element);this.owned.append(wrapper);this.message=d.createElement('p');this.message.setAttribute('role','status');this.owned.append(this.message);
        if(this.mode==='selector')this.renderSelector();else this.renderDetails();
        this.element.focus();
    }
    renderSelector(){
        this.textRun('File:  3univ.dat    Type:  Def',10,1);
        this.textRun(`Number of MO remaining = ${this.remaining}    Number of completed MO found = ${this.completed}`,10,2);
        // TU3511 adds the header above the rectangle's y6 top edge.
        this.textRun('┌'+'─'.repeat(75)+'┐',2,4,0x42);
        this.textRun('│'+' '.repeat(75)+'│',2,5,0x42);
        this.textRun('├'+'─'.repeat(75)+'┤',2,6,0x42);
        for(let y=7;y<=22;y++)this.textRun('│'+' '.repeat(75)+'│',2,y,0x42);
        this.textRun('└'+'─'.repeat(75)+'┘',2,23,0x42);
        this.textRun(centered(TU_SELECTOR_TITLE),3,5,0x47);
        this.textRun(centered(this.single?"Type <CR> or `i' for detailed info, <ESC> to return.":'Choose desired airfield with arrow keys, then use'),3,7,0x4e);
        this.textRun(centered(this.single?'  ':"<CR> or `i' for detailed info, <ESC> to return."),3,8,0x4e);
        for(const f of this.fields){
            if(this.single&&f.index!==4)continue;
            const current=f.index===this.selected,color=current?0x4f:0x47;
            const first=this.elementAt('button',f.first,f.firstX,f.y,color);first.type='button';first.dataset.field=String(f.index);first.setAttribute('aria-label',`${f.first} ${f.last}, ${f.planes} planes, ${f.objectives} mission objectives`);first.setAttribute('aria-pressed',String(current));
            Object.assign(first.style,{border:'0',cursor:'pointer'});first.addEventListener('click',()=>{if(this.busy||!this.active)return;this.selected=f.index;this.mode='details';this.top=0;this.render();this.element.focus();});
            this.textRun(f.last,f.lastX,f.y+1,color);
            this.textRun(`(${f.planes} plane${f.planes===1?'':'s'}  ${f.objectives} MO)`,f.countsX,f.y+2,color);
        }
        this.textRun('MO = Mission Objectives remaining',26,22,0x47);
        this.actions([['Detailed info',()=>this.input(10)],['Return to flight',()=>this.input(27)]]);
    }
    renderDetails(){
        const e=this.game.e,input=e.cc_browser_workspace(),cells=input+512;
        for(let row=0;row<TU_DETAIL_ROWS;row++){
            const text=encodeBytes((this.lines[this.top+row]??'')+'\0');bytes(e,input,text.length).set(text);
            const n=e.cc_towers_markup(input,TU_DETAIL_COLUMNS,cells),raw=bytes(e,cells,n*2);
            let start=0;
            while(start<n){let end=start+1;while(end<n&&raw[end*2+1]===raw[start*2+1])end++;
                const chars=Uint8Array.from({length:end-start},(_,i)=>raw[(start+i)*2]);this.textRun(decodeBytes(chars),2+start,2+row,raw[start*2+1]);start=end;
            }
        }
        this.textRun(tuDetailFooter(this.lines.length),2,24,0x67);
        this.actions([['Previous page',()=>this.input(15)],['Next page',()=>this.input(25)],['Airfields',()=>this.input(10)]]);
    }
    actions(items){
        const row=this.document.createElement('div');row.className='menu-actions';
        for(const [label,action]of items){const b=this.document.createElement('button');b.type='button';b.textContent=label;b.addEventListener('click',()=>this.run(action));row.append(b);}
        this.owned.append(row);
    }
}
