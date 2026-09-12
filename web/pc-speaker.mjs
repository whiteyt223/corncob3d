/* Original 3.ASM F5 no-joystick PC-speaker request: 700 Hz for 25 ms. */
export class PcSpeaker {
    constructor(context=()=>null,{gain=0.08}={}){this.context=context;this.gain=gain;}
    beep(frequency=700,durationMs=25){
        const context=this.context();if(!context||context.state!=='running')return false;
        const start=context.currentTime,stop=start+durationMs/1000,oscillator=context.createOscillator(),level=context.createGain();
        oscillator.type='square';oscillator.frequency.setValueAtTime(frequency,start);level.gain.setValueAtTime(this.gain,start);level.gain.setValueAtTime(this.gain,stop);level.gain.linearRampToValueAtTime(0,stop+0.002);
        oscillator.connect(level).connect(context.destination);oscillator.start(start);oscillator.stop(stop+0.002);return true;
    }
    // Preflight MOAG sound/nosound is an independently gated PIT tone.
    beginTone(frequency){
        this.endTone();const context=this.context();if(!context||context.state!=='running')return false;
        const at=context.currentTime,oscillator=context.createOscillator(),level=context.createGain();
        oscillator.type='square';oscillator.frequency.setValueAtTime(frequency,at);level.gain.setValueAtTime(this.gain,at);
        oscillator.connect(level).connect(context.destination);oscillator.start(at);this.tone={context,oscillator,level};return true;
    }
    endTone(){
        if(!this.tone)return false;const {context,oscillator,level}=this.tone;this.tone=null;
        const at=context.currentTime;level.gain.setValueAtTime(0,at);oscillator.stop(at);return true;
    }
    consume(sortie){if(sortie.cc_sortie_take_beep())return this.beep(700,25);return false;}
}
