/* BSD-3-Clause ymfm YM3812 renderer. The OPL runs at 3,579,545 Hz and native
 * 49,715 Hz; this worklet applies the source register stream and resamples it. */
class CorncobYM3812 extends AudioWorkletProcessor {
    constructor(options){
        super();this.pitHz=options.processorOptions.pitHz;this.events=[];this.tick=0;
        this.e=null;this.rate=0;this.phase=0;this.a=0;this.b=0;this.pending=[];
        this.musicEvents=null;this.musicIndex=0;this.musicFrame=0;this.musicLoopFrames=0;this.musicDurationFrames=0;this.musicPlaybackId=0;
        this.port.onmessage=event=>this.message(event.data);
    }
    async message(message){
        try{
            if(message.type==='wasm'){
                const wasi={args_get(){return 0},args_sizes_get(){return 0},fd_seek(){return 0},fd_write(){return 0},proc_exit(){},fd_close(){return 0}};
                const {instance}=await WebAssembly.instantiate(message.bytes,{wasi_snapshot_preview1:wasi});
                this.e=instance.exports;this.e.cc_ym3812_initialize();this.rate=this.e.cc_ym3812_native_rate();this.resetChip();
                const pending=this.pending;this.pending=[];for(const item of pending)await this.message(item);
                this.port.postMessage({type:'ready'});return;
            }
            if(!this.e){this.pending.push(message);return;}
            if(message.type==='reset'){this.events=[];this.tick=0;this.stopMusic();this.resetChip();}
            else if(message.type==='snapshot'){
                this.events=[];this.tick=message.tick>>>0;this.stopMusic();this.resetChip();
                const registers=new Uint8Array(message.registers);
                for(let register=0;register<registers.length;register++)if(registers[register])this.e.cc_ym3812_write(register,registers[register]);
            }else if(message.type==='events')this.events.push(...message.events);
            else if(message.type==='music-start'){
                this.events=[];this.resetChip();this.musicEvents=message.events;this.musicIndex=0;this.musicFrame=0;this.musicLoopFrames=message.loopFrames>>>0;this.musicDurationFrames=message.durationFrames??((message.events.at(-1)?.[0]??0)+1);this.musicPlaybackId=message.playbackId;
            }else if(message.type==='music-stop'){this.stopMusic();this.resetChip();}
        }catch(error){this.port.postMessage({type:'error',message:String(error?.message||error)});}
    }
    renderOne(){this.e.cc_ym3812_render(1);return new Int16Array(this.e.memory.buffer,this.e.cc_ym3812_buffer(),1)[0]/32768;}
    resetChip(){this.e.cc_ym3812_reset();this.phase=1;this.a=this.b=0;}
    stopMusic(){this.musicEvents=null;this.musicIndex=0;this.musicFrame=0;this.musicLoopFrames=0;this.musicDurationFrames=0;}
    due(eventTick){return (((this.tick>>>0)-(eventTick>>>0))>>>0)<0x80000000;}
    applyMusic(){
        if(!this.musicEvents)return;
        // Restart at the first sample of the next cycle, after rendering every
        // sample in the preceding cycle.
        if(this.musicLoopFrames&&this.musicFrame>=this.musicDurationFrames){this.resetChip();this.musicIndex=0;this.musicFrame=0;}
        while(this.musicIndex<this.musicEvents.length&&this.musicEvents[this.musicIndex][0]<=this.musicFrame){const [,register,value]=this.musicEvents[this.musicIndex++];this.e.cc_ym3812_write(register,value);}
        this.musicFrame++;
        if(!this.musicLoopFrames&&this.musicFrame>=this.musicDurationFrames){
            const playbackId=this.musicPlaybackId;this.stopMusic();this.port.postMessage({type:'music-ended',playbackId});
        }
    }
    process(inputs,outputs){
        const output=outputs[0];
        for(let sample=0;sample<output[0].length;sample++){
            if(this.musicEvents)this.applyMusic();
            else while(this.events.length&&this.due(this.events[0][0])){const [,register,value]=this.events.shift();this.e?.cc_ym3812_write(register,value);}
            let value=0;
            if(this.e){while(this.phase>=1){this.a=this.b;this.b=this.renderOne();this.phase-=1;}value=this.a+(this.b-this.a)*this.phase;this.phase+=this.rate/sampleRate;}
            for(const channel of output)channel[sample]=value;
            this.tick+=this.pitHz/sampleRate;if(this.tick>=0x100000000)this.tick-=0x100000000;
        }
        return true;
    }
}
registerProcessor('corncob-ym3812',CorncobYM3812);
