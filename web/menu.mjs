import {BOSS_RECORD,bossScreen,bossOptions} from './boss.mjs';
import {decodeName} from './characters.mjs';
import {pilotSummary} from './game.mjs';
import {startingAirfield} from './preflight.mjs';
import {theaterGeneralInformation} from './general_information.mjs';
import {inflightIScore} from './iscore-presentation.mjs';
const el=(tag,text,attrs={})=>{const node=document.createElement(tag);if(text!==undefined)node.textContent=text;Object.assign(node,attrs);return node;};
const button=(label,action)=>{const b=el('button',label,{type:'button'});b.addEventListener('click',action);return b;};
const u16=(b,p)=>new DataView(b.buffer,b.byteOffset,b.byteLength).getUint16(p,true);
export class GameMenu {
    constructor(game,root,actions){this.game=game;this.root=root;this.actions=actions;this.busy=false;this.keyHandler=null;}
    screen(title){this.root.removeAttribute('data-boss');this.root.removeAttribute('data-screen');this.root.hidden=false;this.root.replaceChildren();const head=el('div',undefined,{className:'menu-heading'});head.append(el('h2',title));this.root.append(head);this.content=el('div',undefined,{className:'menu-content'});this.root.append(this.content);this.message=el('p','',{className:'menu-message',role:'status'});this.root.append(this.message);this.keyHandler=null;this.backAction=()=>this.game.session?this.actions.resumeMenu?.():this.hangar();return this.content;}
    hide(){this.root.hidden=true;this.keyHandler=null;}
    key(event){
        if(!this.keyHandler){
            if(['INPUT','TEXTAREA','SELECT'].includes(event.target.tagName)||event.ctrlKey||event.altKey||event.metaKey)return;
            if(event.code==='Escape'){event.preventDefault();this.backAction?.();return;}
            const buttons=[...this.root.querySelectorAll('button:not(:disabled)')];
            if(!buttons.length)return;
            const at=buttons.indexOf(document.activeElement);
            if(['ArrowUp','ArrowDown','ArrowLeft','ArrowRight','Home','End'].includes(event.code)){
                event.preventDefault();const n=event.code==='Home'?0:event.code==='End'?buttons.length-1:at<0?0:(at+(['ArrowUp','ArrowLeft'].includes(event.code)?-1:1)+buttons.length)%buttons.length;buttons[n].focus();
            }
            return;
        }
        if(['INPUT','TEXTAREA'].includes(event.target.tagName)&&!this.keyHandler.inputKeys?.has(event.code))return;
        const navigation=['ArrowUp','ArrowDown','ArrowLeft','ArrowRight','Home','End','PageUp','PageDown','Insert','Delete','Enter'];
        const code=event.code.startsWith('Numpad')&&navigation.includes(event.key)?event.key:event.code;
        this.keyHandler(code===event.code?event:{code,repeat:event.repeat,target:event.target,preventDefault:()=>event.preventDefault()});
    }
    async run(action){if(this.busy)return;this.busy=true;this.root.setAttribute('aria-busy','true');try{await action();}catch(error){this.message.textContent=error.message;}finally{this.busy=false;this.root.removeAttribute('aria-busy');}}
    nav(actions){const row=el('div',undefined,{className:'menu-actions'});for(const [label,action] of actions)row.append(button(label,()=>this.run(action)));this.content.append(row);}
    // Wording and shortcut letters follow the preserved MOAG.EXE menu strings.
    choices(items,back=()=>this.hangar(),container=this.content){
        const list=el('div',undefined,{className:'dos-choices'}),buttons=[];
        for(const [label,action] of items){
            const parts=label.split('~'),b=button('',()=>this.run(action));
            b.className='dos-choice';b.append(document.createTextNode(parts[0]));
            if(parts.length>1){b.dataset.hotkey=parts[1][0].toLowerCase();b.append(el('span',parts[1][0],{className:'hotkey'}),document.createTextNode(parts[1].slice(1)));}
            b.addEventListener('focus',()=>{for(const other of buttons)other.classList.toggle('selected',other===b);});
            list.append(b);buttons.push(b);
        }
        container.append(list);buttons[0]?.classList.add('selected');this.backAction=back;
        this.keyHandler=event=>{
            if(event.ctrlKey||event.altKey||event.metaKey)return;
            let at=buttons.findIndex(b=>b.classList.contains('selected'));
            if(['ArrowUp','ArrowDown','ArrowLeft','ArrowRight','Home','End'].includes(event.code)){
                event.preventDefault();at=event.code==='Home'?0:event.code==='End'?buttons.length-1:(at+(['ArrowUp','ArrowLeft'].includes(event.code)?-1:1)+buttons.length)%buttons.length;buttons[at]?.focus();
            }else if(event.code==='Enter'||event.code==='Space'){
                if(event.repeat)return;
                // Tab also reaches the browser's supplementary controls.
                const target=event.target.tagName==='BUTTON'?event.target:buttons[at];event.preventDefault();target?.click();
            }else if(event.code==='Escape'){event.preventDefault();back();}
            else if(!event.repeat){const key=(event.key??event.code.replace(/^Key/,'')).toLowerCase();const b=buttons.find(b=>b.dataset.hotkey===key);if(b){event.preventDefault();b.click();}}
        };
        return list;
    }
    pilotStatus(box=this.content){
        const c=this.game.career,p=pilotSummary(c.current()),t=c.theater(),panel=el('div',undefined,{className:'dos-status'});
        for(const [label,value] of [['Active pilot',`${this.game.assets.pilotLabels?.ranks[p.rank]?.label??''} ${p.name}`],['Theater-of-Operations',c.current()[0x23d]&128?'Training Mission':t?decodeName(t.header.subarray(0,28)):'none']]){
            const line=el('p');line.append(el('span',label+':  ',{className:'status-label'}),document.createTextNode(value));panel.append(line);
        }
        box.append(panel);
        if(this.game.storage?.persistent===false)box.append(el('p','Saves are temporary. Export your pilot records before closing.',{className:'menu-message'}));
        else if(c.active<0)box.append(el('p','Trainee progress lasts for this visit. Create a pilot in the Duty Roster to keep a career.',{className:'muted trainee-note'}));
    }
    hangar(){
        const box=this.screen('Corncob 3D'),ow=!!this.game.e.cc_edition_is_other_worlds?.();this.root.setAttribute('data-screen','main');
        box.append(el('p','Use arrow keys or type highlighted letter to choose action:',{className:'dos-instructions'}));
        const items=[['~Duty roster',()=>this.roster()],['~Combat missions',()=>this.combat()],['~Training missions',async()=>{await this.game.career.selectTraining();this.training();}],['display ~High score list',()=>this.highScores()],['~Read This!!!',()=>this.information()],['keyboard/~Joystick setup',()=>this.actions.joystickOptions?this.actions.joystickOptions():this.controls()],['~Music',()=>this.music()],['~View the Corncob 3D documentation',()=>this.manual()],['get ~Information about this menu',()=>this.information()],['~Browser options',()=>this.browserOptions()]];
        if(ow)items.splice(4,0,['mission ~Editor',()=>this.actions.builder()],['~Sounds editor',()=>this.actions.sounds()],['~Packaged demos',()=>this.actions.demoOptions()]);
        this.choices(items,()=>this.welcome());this.pilotStatus();
    }
    welcome(){
        this.screen('Corncob 3D');this.root.setAttribute('data-screen','welcome');
        this.content.append(el('p',this.game.assets.edition?.label??'Shareware 3.42',{className:'dos-instructions'}),el('p','Kevin Stokes · Pie in the Sky Software',{className:'dos-instructions'}));
        this.choices([['~Begin — original title sequence',()=>this.actions.title()],['~Main menu',()=>this.hangar()]],()=>this.hangar());
    }
    manual(){
        const link=el('a','Open the original manual',{href:this.game.e.cc_edition_is_other_worlds?.()?'editions/other-worlds/manual.txt':'manual.txt',target:'_blank',rel:'noopener'});
        this.screen('Corncob 3D Documentation');this.content.append(link);this.nav([['Flight controls',()=>this.controls()],['Return to Main Menu',()=>this.hangar()]]);
    }
    information(){
        this.screen('Information about Main Menu');this.content.append(el('p','Choose Duty roster to create or activate a pilot. Combat missions opens the Theater of Operations menu; open a theater, then choose Fly mission. Training missions lets you practice and adjust enemy settings.'),el('p','Use the arrow keys and Enter, or type a highlighted letter. Escape returns to the previous menu. You can also click any selection. F1 opens the original boss screen.'),el('p','Browser options contains save import/export, edition selection, the title sequence and full screen.'));
        this.nav([['Return to Main Menu',()=>this.hangar()]]);
    }
    browserOptions(){
        this.screen('Browser options');const items=[['~Export saves',()=>this.actions.export()],['~Import saves',()=>this.actions.import()],['~Full screen',()=>this.actions.fullscreen?.()],['~Title sequence',()=>this.actions.title()]];
        if(this.game.assets.editions?.length>1)items.push(['~Choose edition',()=>this.editions()]);items.push(['~Return to Main Menu',()=>this.hangar()]);this.choices(items);
    }
    combat(){
        this.screen('Theater of Operations Menu');this.root.setAttribute('data-screen','combat');const o=this.game.career.current().subarray(0x234,0x23c),toggle=async kind=>{await this.game.career.setOptions('toggle',kind);this.combat();};
        this.content.append(el('p','Choose with arrow keys or letter',{className:'dos-instructions'}));
        this.choices([['~Fly mission',()=>{if(!this.game.career.theater())throw Error('Open or activate a theater of operations first.');return this.launch();}],['~Open a new theater of operations',()=>this.theaters()],['~Activate an open theater',()=>this.openTheaters('activate')],['~Delete a theater',()=>this.openTheaters('delete')],['~View status of a theater',()=>this.openTheaters('view')],[`toggle invulnerable ~Plane    (${o[7]&8?'Inv':'Std'})`,()=>toggle(2)],[`toggle aerodynamic ~Model    (${o[7]&32?'Advanced':'Beginner'})`,()=>toggle(0)],[`toggle inflight ~Comments    (${o[7]&64?'Off':'On'})`,()=>toggle(1)],['~Return to previous menu',()=>this.hangar()]]);this.pilotStatus();
    }
    theaters(index=0){
        const defs=this.game.assets.theaters,box=this.screen('Choose a Theater of Operations');this.root.setAttribute('data-screen','theaters');
        box.append(el('p','Select a theater with the arrow keys. Press Enter to open it.',{className:'dos-instructions'}));
        const panes=el('div',undefined,{className:'theater-panes'}),left=el('div'),right=el('div',undefined,{className:'theater-detail'});panes.append(left,right);box.append(panes);
        const draw=i=>{const d=defs[i];right.replaceChildren(el('h3',d.name),el('pre',theaterGeneralInformation(this.game.assets.briefings,d.file_stem).join('\n'),{className:'theater-description'}),el('p',`${d.objectives_remaining} objectives · ${d.planes_remaining} aircraft in storage`));};
        const list=this.choices(defs.map(d=>[d.name,async()=>{await this.game.open(d.index);this.combat();}]),()=>this.combat(),left);
        [...list.children].forEach((b,i)=>b.addEventListener('focus',()=>draw(i)));draw(index);
        this.nav([['Return to previous menu',()=>this.combat()]]);
    }
    openTheaters(mode){
        const c=this.game.career,opened=c.openTheaters();this.screen({activate:'Activate an Open Theater',delete:'Delete a Theater',view:'View Theater Records'}[mode]);
        if(!opened.length)this.content.append(el('p','There are no open theaters. Open a new theater from the previous menu.'));
        this.choices([...opened.map(t=>[`${decodeName(t.header.subarray(0,28))} · campaign ${t.id}`,async()=>{if(mode==='activate'){await c.selectTheater(t.index);this.combat();}else if(mode==='delete')this.confirm('Delete a Theater',`Delete ${decodeName(t.header.subarray(0,28))}, campaign ${t.id}?`,async()=>{await c.deleteTheater(t.index);this.openTheaters(mode);},()=>this.openTheaters(mode));else{this.screen('Theater Records');this.content.append(el('h3',decodeName(t.header.subarray(0,28))),el('p',`${u16(t.header,40)} objectives remaining`));this.nav([['Return',()=>this.openTheaters(mode)]]);this.backAction=()=>this.openTheaters(mode);}}]),['~Return to previous menu',()=>this.combat()]],()=>this.combat());
    }
    training(){
        this.options(true);
    }
    editions(){this.screen('Choose edition');this.nav(this.game.assets.editions.map(item=>[item.label,()=>this.actions.changeEdition(item.key)]));this.nav([['Return to Main Menu',()=>this.hangar()]]);}
    async launch(){
        const c=this.game.career,t=c.theater();if(!t){if(!(c.current()[0x23d]&128))throw Error('Select a theater or training mission.');return this.actions.fly(false);}
        const fields=this.game.airfields(),field=fields.rows[4],flags=this.game.e.cc_launch_warning_flags(1,field.planes,field.objectives,fields.remaining),warnings=[];
        if(flags&4){
            // Original MOAG F141..F225: sound precedes the notice text;
            // the dismissal key is consumed before the selector's next read.
            this.screen('Theater is Complete');this.preflightKeys=true;await this.actions.sourceBeep?.();
            for(const text of ['There are no mission objectives remaining in','this Theater of Operations.','Type <ESC> to abort,','or almost anything else to continue...'])this.content.append(el('p',text));
            const choose=key=>key===27?this.hangar():startingAirfield(this,t,fields);
            this.nav([['Continue flying',()=>choose(0)],['Operations',()=>choose(27)]]);
            this.keyHandler=event=>{if(event.repeat||['Shift','Control','Alt','Meta','CapsLock','NumLock','ScrollLock','Dead','Process','Unidentified','Pause','PrintScreen'].includes(event.key))return;event.preventDefault();void this.run(()=>choose(event.moagKey??(event.code==='Escape'?27:0)));};return;
        }
        return startingAirfield(this,t,fields);
    }
    async launchWarnings(theater,flags,index=4,fields=this.game.airfields()){
        const kind=flags&1?1:flags&2?2:0;if(!kind)return this.actions.fly(false,index);
        const stores=new DataView(theater.header.buffer,theater.header.byteOffset).getInt16(38,true);
        this.screen(kind===1?'No Objectives at this Airfield':'No Planes at this Airfield');this.preflightKeys=true;await this.actions.sourceBeep?.();
        const lines=kind===1?['Warning! There are no uncompleted mission objectives at this airstrip.','Type <CR> to fly anyway, <ESC> to abort,','or almost anything else to choose a different airstrip.']:
            ['Warning!  There are no planes available at this airstrip.',...(stores!==0?[`        (However, you have ${stores} plane${stores===1?'':'s'} in storage.)`]:[]),...(stores>0?["        Type 'P' to pull a new plane from storage."]:[]),'      Type <CR> to continue with the mission anyway.','                    Type <ESC> to abort.','       Type almost anything else to select a new tower.'];
        for(const text of lines)this.content.append(el('p',text));
        const choose=async key=>{const result=this.game.e.cc_launch_warning_choice(kind,stores,key);if(result===2)return this.actions.fly(true,index);if(result===1)return this.launchWarnings(theater,flags&~kind,index,fields);if(result===0)return startingAirfield(this,theater,fields,index);this.hangar();};
        const actions=[['Continue anyway',()=>choose(13)]];if(kind===2&&stores>0)actions.unshift(['Pull a plane from storage · P',()=>choose(112)]);actions.push(['Cancel',()=>choose(27)]);this.nav(actions);
        this.keyHandler=event=>{if(event.repeat||['Shift','Control','Alt','Meta','CapsLock','NumLock','ScrollLock','Dead','Process','Unidentified','Pause','PrintScreen'].includes(event.key))return;const key=event.moagKey??(event.code==='Enter'?13:event.code==='Escape'?27:event.code==='KeyP'?112:0);event.preventDefault();void this.run(()=>choose(key));};
    }
    options(trainingMenu=false){
        const c=this.game.career,p=c.current(),o=p.subarray(0x234,0x23c),box=this.screen(trainingMenu?'Training Missions':'Flight options'),training=!!(p[0x23d]&128);
        const toggle=kind=>this.run(async()=>{await c.setOptions('toggle',kind);this.options(trainingMenu);});
        if(trainingMenu)this.nav([['Fly training mission',()=>this.launch()]]);
        box.append(button(`Aerodynamic model: ${o[7]&32?'Advanced':'Beginner'}`,()=>toggle(0)),button(`Inflight comments: ${o[7]&64?'Off':'On'}`,()=>toggle(1)),button(`Combat invulnerability: ${o[7]&8?'On':'Off'}`,()=>toggle(2)),button(`Training invulnerability: ${o[7]&2?'On':'Off'}`,()=>toggle(3)));
        box.append(el('h3','Training enemies'),el('p','Wickedness 0 gives the original random spread; 1–8 selects a strength.',{className:'muted'}));
        for(const setting of this.game.assets.pilotLabels.enemy_settings){const label=el('label',`${setting.label} ${setting.setting} `),input=el('input',undefined,{type:'number',min:String(setting.minimum),max:String(setting.maximum),step:'1',value:String(o[setting.field])});label.append(input);box.append(label);input.addEventListener('change',()=>this.run(async()=>{const n=Number(input.value);if(!Number.isInteger(n))throw Error('Enter a whole number.');await c.setOptions('enemy',setting.field,n);}));}
        this.nav([['Original defaults',async()=>{await c.setOptions('defaults');this.options(trainingMenu);}],['Return to Main Menu',()=>this.hangar()]]);
    }
    highScores(list=0){
        const labels=['Missions','Careers','Fastest careers'],c=this.game.career,bytes=c.highScores(),base=[0,1800,3420][list],stride=list?162:180,box=this.screen(`High scores · ${labels[list]}`),table=el('table'),head=el('tr');
        const columns=list===0?['Pilot','Theater','Score']:list===1?['Pilot','Sorties','Score']:['Pilot','Sorties','Score per sortie'];for(const label of columns)head.append(el('th',label));table.append(head);
        for(let i=0;i<10;i++){const row=bytes.subarray(base+i*stride,base+(i+1)*stride);if(!row[0])break;const v=new DataView(row.buffer,row.byteOffset,row.length),count=list?v.getInt16(22,true):0,score=v.getInt32(list?156:174,true),name=decodeName(row.subarray(0,22))+(u16(row,stride-2)?'*':''),values=[name,list?count:decodeName(row.subarray(22)),list===2?(count?Math.trunc(score/count):0):score],tr=el('tr');for(const value of values)tr.append(el('td',typeof value==='number'?value.toLocaleString():value));table.append(tr);}
        box.append(table,el('p','* Deleted pilot',{className:'muted'}));this.nav([...labels.map((label,index)=>[label,()=>this.highScores(index)]),['Clear this list',()=>this.confirm('Clear high scores',`Clear the ${labels[list].toLowerCase()} list?`,async()=>{await c.clearScores(list);this.highScores(list);},()=>this.highScores(list))],['Return to Main Menu',()=>this.hangar()]]);
    }
    music(){this.screen('Original music');this.content.append(el('p','Select your original .ROL tracks and their .BNK instrument bank to play the menu music.'));this.nav([['Choose music files',()=>this.actions.importMusic()],['Play menu music',()=>this.actions.playMusic()],['Stop music',()=>this.actions.stopMusic()],['Return to Main Menu',()=>this.hangar()]]);}
    roster(){
        this.screen('Duty Roster');this.root.setAttribute('data-screen','roster');
        this.content.append(el('p','Use arrow keys or type highlighted letter to choose action:',{className:'dos-instructions'}));
        this.choices([['~Create Pilot',()=>this.createPilot()],['~Activate Pilot',()=>this.pilotList('activate')],['~Deactivate Pilot',async()=>{await this.game.career.activate(-1);this.roster();}],['de~Lete Pilot',()=>this.pilotList('delete')],['r~Esurrect Pilot',()=>this.pilotList('resurrect')],['re~Name Pilot',()=>this.pilotList('rename')],['~View Pilot Records',()=>this.pilotList('view')],['~Return to previous menu',()=>this.hangar()]]);this.pilotStatus();
    }
    createPilot(){
        this.screen('Create New Pilot');this.content.append(el('p','Enter name for new pilot. (Maximum of 20 characters)'));
        const form=el('form'),name=el('input',undefined,{type:'text',maxLength:20,required:true});name.setAttribute('aria-label','New pilot name');
        form.append(name,el('button','Create Pilot',{type:'submit'}));form.addEventListener('submit',event=>{event.preventDefault();void this.run(async()=>{await this.game.career.create(name.value);this.roster();});});this.content.append(form);this.nav([['Cancel',()=>this.roster()]]);this.backAction=()=>this.roster();name.addEventListener('keydown',event=>{if(event.code==='Escape'){event.preventDefault();this.roster();}});name.focus();
    }
    pilotList(mode){
        const c=this.game.career;this.screen({activate:'Choose Active Pilot',delete:'Choose Pilot to Delete',resurrect:'Choose Pilot to Resurrect',rename:'Choose Pilot to Rename',view:'View Pilot Records'}[mode]);
        const pilots=c.pilots.map((p,i)=>({p,i}));if(mode==='view')pilots.unshift({p:c.trainee,i:-1});
        if(!pilots.length)this.content.append(el('p','There are no pilots in the list. Create a pilot in the Duty Roster first.'));
        this.choices([...pilots.map(({p,i})=>[`${pilotSummary(p).name}${c.active===i?'  (active)':''}`,async()=>{
            if(mode==='activate'){const result=await c.activate(i);if(result?.resurrection)this.resurrect(i);else this.roster();}
            else if(mode==='rename')this.rename(i);
            else if(mode==='view')this.record(p,()=>this.pilotList('view'));
            else this.confirm(mode==='delete'?'Delete Pilot':'Resurrect Pilot',`${mode==='delete'?'Delete':'Resurrect'} ${pilotSummary(p).name} and their theater records?`,async()=>{if(mode==='delete')await c.deletePilot(i);else await c.resurrect(i);this.roster();},()=>this.pilotList(mode));
        }]),['~Return to previous menu',()=>this.roster()]],()=>this.roster());
    }
    confirm(title,text,accept,cancel=()=>this.roster()){this.screen(title);this.content.append(el('p',text));this.nav([['Confirm',accept],['Cancel',cancel]]);this.keyHandler=event=>{if(event.code==='Enter'){event.preventDefault();void this.run(accept);}if(event.code==='Escape'){event.preventDefault();cancel();}};}
    rename(index){this.screen('Rename pilot');const form=el('form'),name=el('input',undefined,{type:'text',maxLength:20,required:true,value:pilotSummary(this.game.career.pilots[index]).name});form.append(name,el('button','Save name',{type:'submit'}));form.addEventListener('submit',event=>{event.preventDefault();void this.run(async()=>{await this.game.career.rename(index,name.value);this.roster();});});this.content.append(form);this.nav([['Cancel',()=>this.roster()]]);name.focus();name.select();}
    async deleteTheater(theater){const remove=async()=>{await this.game.career.deleteTheater(theater.index);this.hangar();};if(u16(theater.header,46))this.confirm('Delete completed theater','Delete this completed theater?',remove,()=>this.hangar());else await remove();}
    resurrect(index){this.screen('Return pilot to duty');this.content.append(el('p',`${pilotSummary(this.game.career.pilots[index]).name} was killed or captured. Resurrect this pilot?`));this.nav([['Resurrect and activate',async()=>{await this.game.career.activate(index,{resurrect:true});this.roster();}],['Cancel',()=>this.roster()]]);}
    record(p=this.game.career.current(),back=()=>this.roster()){const s=pilotSummary(p),labels=this.game.assets.pilotLabels,box=this.screen(`${s.name} · pilot record`),table=el('table');for(const [key,value] of [['Rank',labels?.ranks[s.rank]?.label??String(s.rank)],['Sorties',s.sorties],['Total score',s.score.toLocaleString()],['Best score',new DataView(p.buffer,p.byteOffset).getInt32(0x124,true).toLocaleString()],['Resurrections',u16(p,0x1a)]]){const tr=el('tr');tr.append(el('th',key),el('td',String(value)));table.append(tr);}box.append(table);const awards=(labels?.medals??[]).filter(m=>p[0x257+m.index]);if(awards.length){box.append(el('h3','Decorations'));for(const medal of awards)box.append(el('p',`${medal.label} × ${p[0x257+medal.index]}`));}this.nav([['Return to previous menu',back]]);this.backAction=back;}
    tower(){
        const briefing=this.game.briefing(),box=this.screen('Control tower'),pre=el('pre','',{className:'briefing','aria-label':'Original tower briefing'}),label=el('p','',{className:'muted'});let line=0;
        const redraw=()=>{line=Math.max(0,Math.min(line,Math.max(0,briefing.lines.length-22)));const visible=briefing.lines.slice(line,line+22);pre.textContent=visible.map(s=>s.startsWith('Sxyth ')?s.slice(6):s).join('\n');const marker=visible.find(s=>s.startsWith('Sxyth '));this.coordinates=marker?marker.slice(6).trim().split(/\s+/).slice(0,4).map(Number):null;label.textContent=`Lines ${line+1}–${Math.min(line+22,briefing.lines.length)}${this.coordinates?' · Enter accepts the first visible mission position':''}`;};
        box.append(pre,label);this.nav([['Previous page',()=>{line-=22;redraw();}],['Next page',()=>{line+=22;redraw();}],['Accept / return',()=>this.actions.leaveTower(this.coordinates)],['Cancel',()=>this.actions.leaveTower(null)]]);
        this.keyHandler=event=>{if(['ArrowUp','ArrowDown','PageUp','PageDown','Home','End','Enter','Escape'].includes(event.code)){event.preventDefault();if(event.code==='Enter')this.actions.leaveTower(this.coordinates);else if(event.code==='Escape')this.actions.leaveTower(null);else{line+=({ArrowUp:-1,ArrowDown:1,PageUp:-22,PageDown:22,Home:-briefing.lines.length,End:briefing.lines.length})[event.code];redraw();}}};redraw();
    }
    sourceNotice(kind){
        const boss=kind==='boss',text=kind==='score-unavailable'?'Sorry, inflight score not available in this version':kind==='theater-unavailable'?'Sorry, inflight theater detailed info not available in this version':kind==='calibration'?'Keyboard controls are active.':'C:\\>';
        this.screen(boss?'':'Corncob 3D');if(boss){this.root.setAttribute('data-boss','');const terminal=bossScreen(this.game.career.records.get(BOSS_RECORD));this.content.append(terminal);}else this.content.append(el('p',text));
        this.nav([['Return to flight',()=>this.actions.resumeMenu()]]);
        this.keyHandler=event=>{if(!event.repeat&&!['ShiftLeft','ShiftRight','ControlLeft','ControlRight','AltLeft','AltRight','MetaLeft','MetaRight','CapsLock','NumLock','ScrollLock'].includes(event.code)){event.preventDefault();this.actions.resumeMenu();}};
    }
    inflightScore(raw){return inflightIScore(this,raw);}
    report(report,{preview=false}={}){
        this.screen(preview?'Current sortie report':'Sortie report');this.content.append(el('p',`Score: ${report.score.score.toLocaleString()}`,{className:'report-score'}));
        if(!preview){this.content.append(el('p',report.accepted?'The sortie has been recorded.':report.reason===1?'This invulnerable sortie does not count toward your career.':'This sortie was not recorded.'));if(report.stockade)this.content.append(el('p','This pilot has been placed in the stockade for friendly casualties.'));if(report.promoted)this.content.append(el('p',`Promoted to ${this.game.assets.pilotLabels?.ranks[report.rank]?.label??'the next rank'}.`));for(const [index,count] of (report.awards??[]).entries())if(count)this.content.append(el('p',`${this.game.assets.pilotLabels?.medals[index]?.label??'Award'} × ${count}`));}
        const actions=[[preview?'Return to flight':'Operations',()=>preview?this.actions.resumeMenu():this.hangar()]];
        if(!preview&&this.game.storage?.persistent===false){this.content.append(el('p','These saves are temporary. Export them before closing this page.'));actions.unshift(['Export saves',()=>this.actions.export()]);}
        this.nav(actions);
        this.root.querySelectorAll('button:not(:disabled)')[0]?.focus();
    }
    errorChoice(){this.screen('Sortie error');this.backAction=()=>{};this.content.append(el('p','The engine reported an error. Do you want this mission recorded?'));this.nav([['Record mission',()=>this.actions.finish(1)],['Do not record',()=>this.actions.finish(2)]]);}
    controls(){
        this.screen('Flight controls');this.nav([['Customize boss screen',()=>bossOptions(this)]]);if(this.actions.joystickOptions)this.nav([['Input device',()=>this.actions.joystickOptions()]]);const table=el('table');for(const [key,action] of [['↑ / ↓','Pitch; down pulls the nose up'],['← / →','Roll'],['Z / X','Rudder'],['+ / −','Throttle'],['. / Insert','Brakes'],['Space','Machine gun aboard; handgun on foot; release opens parachute after ejection'],['Left Shift + Space','Fire cannons'],['C','Missiles'],['B','Drop an aircraft bomb'],[';','Plant an assassin bomb while ejected'],['D','Arm and release planted assassin bombs'],['F','Flaps'],['A','Autopilot'],['E','Eject or step out'],['J','Hold to charge a jump; release to jump while on foot'],['V','Call a rescue van while on foot'],['I','Ignite the rocket booster when charged'],['Caps Lock','Toggle remote aircraft control while ejected'],['Num Lock','Toggle arrows between controls and views'],['Left Ctrl + arrows','Look around while held'],['Numpad 5 / Insert','Walk forward / backward'],['R','Rear view'],['L','Full view'],['G','Toggle ground detail'],['S','Toggle sound'],['T','Hold to accelerate time'],['F5','Recalibrate joystick'],['F2','Control tower and mission briefings'],['F1','Original boss screen'],['F4','Open original help'],['F3 / F6','Shareware availability notices'],['M','Paused 3D map; Home centers; Page Up / Down zoom'],['P','Pause and help; Space cycles help pages'],['Esc','End sortie (warning when away from base)'],['Ctrl C','Abort sortie']]){const row=el('tr');row.append(el('th',key),el('td',action));table.append(row);}this.content.append(table);const a=el('a','Read the original manual',{href:'manual.txt',target:'_blank',rel:'noopener'});this.content.append(a);this.nav([[this.game.session?'Return':'Operations',()=>this.game.session?this.actions.resumeMenu():this.hangar()]]);
    }
    failure(text,retry,recover){
        this.screen('Could not finish saving');this.backAction=()=>{};this.content.append(el('p',text));const actions=[['Retry save',retry]];
        if(recover){this.content.append(el('p','Continue saving for this visit. Named pilot saves can be exported; the trainee remains temporary. Existing browser saves will remain unchanged.'));actions.push(['Continue with temporary saves',recover]);}
        this.nav(actions);
    }
}
