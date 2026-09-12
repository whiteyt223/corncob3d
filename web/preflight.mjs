import {moagPreflightKey} from './moag-menu-keys.mjs';
// Original MOAG EF63 airfield selector and160DE mode7 detail viewer.
const el=(tag,text,attrs={})=>{const n=document.createElement(tag);if(text!==undefined)n.textContent=text;Object.assign(n,attrs);return n;};
const bytes=(e,p,n)=>new Uint8Array(e.memory.buffer,p,n);
const cstring=(e,p)=>{const all=new Uint8Array(e.memory.buffer);let end=p;while(all[end])end++;return new TextDecoder().decode(all.subarray(p,end));};
const fieldName=(e,i)=>`${cstring(e,e.cc_towers_field_name(i,0))} ${cstring(e,e.cc_towers_field_name(i,1))}`;
export function preflightKey(event){const key=event.moagKey??moagPreflightKey(event);return key<0?null:key;}
export function detailedInfo(menu,theater,fields,index){
    const e=menu.game.e,p=e.cc_world_file_buffer();bytes(e,p,theater.bytes.length).set(theater.bytes);
    if(e.cc_towers_generate(p,theater.bytes.length,671))throw Error('Could not prepare the original airfield information.');
    const raw=new TextDecoder().decode(bytes(e,e.cc_towers_text(),e.cc_towers_text_size()));
    const section=raw.split('\f\r\n')[index+1]??'',lines=section.split('\r\n');if(lines.at(-1)==='')lines.pop();
    const box=menu.screen('Detailed Info'),pre=el('pre','',{className:'briefing','aria-label':'Original airfield details'}),status=el('p','',{className:'muted'});box.append(pre,status);let top=0;
    menu.preflightKeys=true;
    const close=()=>startingAirfield(menu,theater,fields,index);
    const draw=()=>{
        pre.replaceChildren();
        for(let row=0;row<15;row++){
            const text=new TextEncoder().encode((lines[top+row]??'')+'\0'),input=e.cc_browser_workspace(),cells=input+256;bytes(e,input,text.length).set(text);
            const length=e.cc_towers_markup(input,73,cells),data=bytes(e,cells,length*2);let part='',color=-1;
            const flush=()=>{if(!part)return;const span=el('span',part);if(color===0x6f)span.style.fontWeight='bold';if(color===0x6e)span.style.color='#ffdf65';pre.append(span);part='';};
            for(let i=0;i<length;i++){if(data[i*2+1]!==color){flush();color=data[i*2+1];}part+=String.fromCharCode(data[i*2]);}flush();if(row<14)pre.append(document.createTextNode('\n'));
        }
        status.textContent=`Lines ${top+1}–${Math.min(top+15,lines.length)} of ${lines.length}`;
    };
    const choose=async key=>{const result=e.cc_towers_view_key(top,lines.length,key);top=result&65535;if(result&0x20000)await menu.actions.sourceBeep?.();if(result&0x10000)close();else draw();};
    menu.nav([['Previous page',()=>choose(15)],['Next page',()=>choose(25)],['Return',()=>choose(27)]]);
    menu.keyHandler=event=>{const key=preflightKey(event);if(key===null||event.repeat)return;event.preventDefault();void menu.run(()=>choose(key));};draw();
}
export function startingAirfield(menu,theater,fields,index=null){
    if(index===null){const e=menu.game.e,p=e.cc_world_file_buffer();bytes(e,p,theater.bytes.length).set(theater.bytes);index=e.cc_towers_initial(p,theater.bytes.length,Number(fields.single));if(index<0||index>8)throw Error('The saved position is outside the original airfield selector.');}
    const e=menu.game.e,box=menu.screen(fields.single?'Your Starting Airfield':'Choose New Starting Airfield');
    menu.preflightKeys=true;
    box.append(el('p',fields.single?'There is only one airfield in this Theater-of-Operation.':'Choose new starting airfield with arrow keys, then use'));
    const table=el('table'),head=el('tr');for(const text of ['Airfield','Planes','Objectives'])head.append(el('th',text));table.append(head);
    for(const f of fields.rows){if(fields.single&&f.index!==4)continue;const row=el('tr'),name=el('button',fieldName(e,f.index),{type:'button'});name.addEventListener('click',()=>menu.run(()=>startingAirfield(menu,theater,fields,f.index)));if(f.index===index)name.setAttribute('aria-current','true');const cell=el('td');cell.append(name);row.append(cell,el('td',String(f.planes)),el('td',String(f.objectives)));table.append(row);}box.append(table);
    box.append(el('p',fields.single?"Type <CR> to fly, `i' for detailed info, or <ESC> to abort mission.":"<CR> to select, `i' for detailed info, or <ESC> to abort mission."));
    const choose=async key=>{
        if(key===27)return menu.hangar();
        // Backspace returns the original selector argument before its warning
        // and X/Y-write branches; the prepared file position is retained.
        if(key===8)return menu.actions.fly(false);
        if(key===10){const f=fields.rows[index],flags=e.cc_launch_warning_flags(1,f.planes,f.objectives,fields.remaining);return menu.launchWarnings(theater,flags&3,index,fields);}
        if(key===105)return detailedInfo(menu,theater,fields,index);
        const result=e.cc_towers_airfield_key(index,Number(fields.single),key);if(result&0x20000)await menu.actions.sourceBeep?.();if((result&65535)!==index)return startingAirfield(menu,theater,fields,result&65535);
    };
    menu.nav([['Fly',()=>choose(10)],['Detailed info · I',()=>choose(105)],['Abort mission',()=>choose(27)]]);
    menu.keyHandler=event=>{const key=preflightKey(event);if(key===null||event.repeat)return;event.preventDefault();void menu.run(()=>choose(key));};
}
