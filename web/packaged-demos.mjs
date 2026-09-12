import {validateDemo} from './demo-stream.mjs';
// The original MOAG menu wraps its three shipped entries, using its common
// reader's Up/Left and Down/Right codes. Enter launches; Esc/Backspace leave.
export function packagedDemoKey(index,key,count){
    if(key===10)return {index,action:'play'};
    if(key===27||key===8)return {index,action:'exit'};
    if(key===16||key===2)return {index:(index+count-1)%count,action:'select'};
    if(key===14||key===6)return {index:(index+1)%count,action:'select'};
    return {index,action:'beep'};
}
export async function loadPackagedDemos(url,{json,binary}){
    if(!url)return null;
    const catalog=await json(url),base=url.slice(0,url.lastIndexOf('/')+1);
    if(catalog.format!=='original-other-worlds-packaged-demos-v1')throw Error('Unsupported original demo catalog.');
    const files=new Map(await Promise.all(catalog.records.map(async row=>{
        const bytes=await binary(base+row.file),framing=validateDemo(bytes);
        if(bytes.length!==row.bytes||framing.records!==row.framing.records||framing.consumed!==row.framing.consumed)throw Error(`Incomplete packaged demo: ${row.name}.`);
        return [row.stem,bytes];
    })));
    return {catalog,files};
}
export function packagedDemoOptions(menu,index=0){
    const packaged=menu.game.assets.packagedDemos,rows=packaged?.catalog.records??[];
    if(!menu.game.e.cc_edition_is_other_worlds?.()||!rows.length)throw Error('No original packaged demos are installed.');
    index=Math.max(0,Math.min(rows.length-1,index));const row=rows[index],box=menu.screen('Choose a Demo Flight');menu.preflightKeys=true;
    const add=(tag,text)=>{const node=document.createElement(tag);node.textContent=text;box.append(node);return node;};
    add('p','Choose the desired demo from the list using the arrow keys. Press Enter to run the demo, or Esc to abort.');
    add('h3','Available Demos:');menu.nav(rows.map((r,i)=>[r.name,()=>packagedDemoOptions(menu,i)]));
    add('p',`Demo: ${row.name}`);const theater=menu.game.assets.theaters.find(t=>t.file_stem===row.world_stem);add('p',`Set in theater: ${theater?.name??row.world_stem}`);
    add('pre',row.description_lines.join('\n')).className='theater-description';
    const play=()=>menu.actions.playPackagedDemo(row.stem);menu.nav([['Watch demo',play],['Demos',()=>menu.actions.demoOptions()]]);
    menu.keyHandler=event=>{event.preventDefault();const result=packagedDemoKey(index,event.moagKey,rows.length);
        if(result.action==='select')packagedDemoOptions(menu,result.index);
        else if(result.action==='exit')menu.actions.demoOptions();
        else if(result.action==='play')void menu.run(play);
        else void menu.run(()=>menu.actions.sourceBeep?.());
    };
}
export function packagedDemoLaunch(assets,stem){
    if(assets.editionId!==2)throw Error('Packaged demos are available in Other Worlds.');
    const catalog=assets.packagedDemos,row=catalog?.catalog.records.find(r=>r.stem===stem),bytes=catalog?.files.get(stem);
    if(!row||!bytes)throw Error('This original demo is not installed.');
    const index=assets.theaters.findIndex(t=>t.file_stem===row.world_stem),world=assets.worlds.get(row.world_stem);
    if(index<0||!world)throw Error('The original demo world is missing.');
    // MOAGCC9F copies the unmodified DEF directly into the temporary universe;
    // it creates no THT, changes no selected career and passes only -rbsk.
    return {packagedDemo:{stem:row.stem,name:row.name,header:assets.definitions.slice(index*48,index*48+48),world:world.slice()},demoPlayback:bytes.slice()};
}
export function applyPackagedDemoFlags(ds){
    // Original OW command leaves DCB0(r), DC9B(b), DCA9(s), DC59(k).
    ds[0xaf1]=255;ds[0xaeb]=255;ds[0xaf0]=255;ds[0xaf6]=0;
}
export async function finishPackagedDemo(game){
    const session=game.session;if(!session?.packagedDemo||!session.exit)throw Error('No completed packaged demo is available.');
    // 3.EXE still writes 3D.CFG; MOAG10CC3 skips career scoring and removes the
    // temporary world/demo. Commit just the configuration, then publish it.
    const config=session.exit.config;
    if(config?.length===52){const copy=config.slice();if(game.career.storage)await game.career.storage.commit(new Map([['3d.cfg',copy]]));game.career.records.set('3d.cfg',copy);game.assets.config=copy.slice();}
    return {demo:true,engineError:session.exit.engineError??0};
}
