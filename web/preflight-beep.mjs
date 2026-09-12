// Original MOAG image D832: BIOS-tick waits around sound(622)/nosound.
// sound3332 programs divisor floor(0x1234DD / 622) on PIT channel2.
export const BIOS_TICK_MS=65536*1000/1193182;
export const PREFLIGHT_DIVISOR=Math.floor(0x1234dd/622);
export const PREFLIGHT_FREQUENCY=1193182/PREFLIGHT_DIVISOR;
export function preflightBiosClock(){
    const d=new Date(),ms=((d.getHours()*60+d.getMinutes())*60+d.getSeconds())*1000+d.getMilliseconds(),ticks=ms/BIOS_TICK_MS;
    return {tick:Math.floor(ticks)>>>0,delay:(Math.floor(ticks)+1-ticks)*BIOS_TICK_MS};
}
// The executable compares the high word signed, then the low word unsigned.
const before=(a,b)=>(a|0)<(b|0);
export class PreflightBeepState {
    constructor(tick){this.start=tick>>>0;this.stage=1;}
    update(tick,pending){
        const events=[];
        if(this.stage===1&&(!before(tick,(this.start+1)>>>0)||pending)){this.stage=2;events.push('sound');}
        if(this.stage===2&&(!before(tick,(this.start+2)>>>0)||pending)){this.stage=0;events.push('nosound');}
        return events;
    }
}
const modifiers=new Set(['Shift','Control','Alt','Meta','CapsLock','NumLock','ScrollLock','Dead','Process','Unidentified','Pause','PrintScreen']);
const dosKey=event=>!!event.key&&!modifiers.has(event.key);
export class PreflightBeep {
    constructor(speaker,{clock=preflightBiosClock,schedule=(fn,ms)=>setTimeout(fn,ms),cancel=id=>clearTimeout(id)}={}){
        this.speaker=speaker;this.clock=clock;this.schedule=schedule;this.cancel=cancel;this.menu=null;this.state=null;this.keys=[];this.timer=null;this.drainTimer=null;this.resolve=null;
    }
    play(menu){
        if(this.state)throw Error('Overlapping preflight beeps.');
        this.menu=menu;this.state=new PreflightBeepState(this.clock().tick);
        const done=new Promise(resolve=>{this.resolve=resolve;});this.pump();return done;
    }
    capture(menu,event){
        if(this.menu!==menu||!dosKey(event))return false;
        event.preventDefault();
        // kbhit does not remove the pending key. A repeated key is also a
        // queued DOS character, so replay it once after this wait returns.
        this.keys.push({code:event.code,key:event.key,repeat:false,shiftKey:event.shiftKey,ctrlKey:event.ctrlKey,altKey:event.altKey,metaKey:event.metaKey,target:event.target,preventDefault(){}});
        if(this.state)this.pump();else this.scheduleDrain();
        return true;
    }
    pump(){
        if(this.timer!==null){this.cancel(this.timer);this.timer=null;}
        const now=this.clock(),events=this.state.update(now.tick,this.keys.length!==0);
        for(const event of events)if(event==='sound')this.speaker.beginTone(PREFLIGHT_FREQUENCY);else this.speaker.endTone();
        if(this.state.stage){this.timer=this.schedule(()=>{this.timer=null;this.pump();},Math.max(0,now.delay));return;}
        this.state=null;const resolve=this.resolve;this.resolve=null;resolve();this.scheduleDrain();
    }
    scheduleDrain(){
        if(this.drainTimer!==null)return;
        // A macrotask allows the awaiting choice and GameMenu.run's finally
        // to finish before the preserved BIOS key reaches the next read.
        this.drainTimer=this.schedule(()=>{this.drainTimer=null;this.drain();},0);
    }
    drain(){
        if(this.state)return;
        if(this.menu.busy){this.scheduleDrain();return;}
        if(this.menu.root.hidden){this.keys.length=0;this.menu=null;return;}
        if(!this.keys.length){this.menu=null;return;}
        const event=this.keys.shift();this.menu.key(event);
        if(!this.state)this.scheduleDrain();
    }
}
