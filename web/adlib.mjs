// Browser bridge for core/audio.cpp. OPL time is the source raw PIT time, not
// AudioContext time. A snapshot is used whenever the consumer could have missed
// register events (initial start, pause/resume, or ring overwrite).
export class AdlibAudio {
    constructor(exports,{pitHz=1193182/1024}={}){
        this.e=exports;this.pitHz=pitHz;this.node=null;this.context=null;
        this.sequence=0;this.ready=false;this.resyncNeeded=true;this.musicActive=false;this.musicGeneration=0;this.musicPlaybackId=0;this.onMusicEnded=null;
    }
    start(){
        if(this.node)return Promise.resolve();
        if(this.startPending)return this.startPending;
        const pending=this.startOnce();
        this.startPending=pending;
        // Keep one shared initialization task for every caller; expose the same
        // rejection and permit a later user gesture to retry a failed start.
        const clear=()=>{if(this.startPending===pending)this.startPending=null;};
        pending.then(clear,clear);
        return pending;
    }
    async startOnce(){
        let node,context;
        try{
            context=new AudioContext({latencyHint:'interactive'});
            const wasm=await (await fetch('ym3812.wasm')).arrayBuffer();
            await context.audioWorklet.addModule('ymfm-opl2-processor.mjs');
            node=new AudioWorkletNode(context,'corncob-ym3812',{processorOptions:{pitHz:this.pitHz}});
            node.connect(context.destination);
            await new Promise((resolve,reject)=>{
                const timeout=setTimeout(()=>reject(new Error('YM3812 worklet initialization timed out')),10000);
                const finish=(error)=>{clearTimeout(timeout);node.port.onmessage=null;node.onprocessorerror=null;error?reject(error):resolve();};
                node.port.onmessage=event=>{const m=event.data;if(m?.type==='ready'){this.ready=true;finish();}else if(m?.type==='error')finish(new Error(`YM3812 worklet: ${m.message||'initialization failed'}`));};
                node.onprocessorerror=()=>finish(new Error('YM3812 worklet processor error'));
                node.port.postMessage({type:'wasm',bytes:wasm},[wasm]);
            });
            node.port.onmessage=event=>{const m=event.data;if(m?.type==='music-ended'&&this.musicActive&&m.playbackId===this.musicPlaybackId){this.musicActive=false;this.musicPlaybackId=0;this.resyncNeeded=true;this.onMusicEnded?.(m.playbackId);}};
            this.context=context;this.node=node;await context.resume();this.resync();
        }catch(error){
            this.ready=false;this.node=null;this.context=null;node?.disconnect();await context?.close().catch(()=>{});throw error;
        }
    }
    reset(){this.stopMusic();this.sequence=0;this.resyncNeeded=true;if(this.ready)this.resync();}
    async pause(){if(!this.context)return;if(!this.musicActive)this.resyncNeeded=true;await this.context.suspend();}
    async resume(){if(!this.context)return;await this.context.resume();if(!this.musicActive)this.resync();}
    resync(){
        if(!this.node||!this.ready||this.musicActive)return;
        const end=this.e.cc_audio_event_sequence()>>>0;
        const registers=new Uint8Array(this.e.memory.buffer,this.e.cc_audio_register_ptr()>>>0,256).slice();
        this.node.port.postMessage({type:'snapshot',tick:this.e.cc_audio_raw_ticks()>>>0,registers:registers.buffer},[registers.buffer]);
        this.sequence=end;this.resyncNeeded=false;
    }
    flush(){
        if(!this.node||!this.ready||this.musicActive)return;
        if(this.context.state!=='running'){this.resyncNeeded=true;return;}
        if(this.resyncNeeded){this.resync();return;}
        const end=this.e.cc_audio_event_sequence()>>>0,capacity=this.e.cc_audio_event_capacity()>>>0;
        if(((end-this.sequence)>>>0)>capacity){this.resync();return;}
        const ptr=this.e.cc_audio_event_ptr()>>>0,view=new DataView(this.e.memory.buffer),batch=[];
        for(;this.sequence!==end;this.sequence=(this.sequence+1)>>>0){
            const at=ptr+(this.sequence%capacity)*12;
            if(view.getUint32(at,true)!==this.sequence){this.resync();return;}
            batch.push([view.getUint32(at+4,true),view.getUint8(at+8),view.getUint8(at+9)]);
        }
        if(batch.length)this.node.port.postMessage({type:'events',events:batch});
    }
    async playRol(rolUrl='DESTINY3.ROL',bankUrl='CCMUSIC.BNK',{loop=false}={}){
        const playbackId=++this.musicGeneration;
        await this.start();
        const [{RolMusic},rol,bank]=await Promise.all([import('./rol-music.mjs'),fetch(rolUrl).then(r=>{if(!r.ok)throw new Error(`unable to load ${rolUrl}`);return r.arrayBuffer();}),fetch(bankUrl).then(r=>{if(!r.ok)throw new Error(`unable to load ${bankUrl}`);return r.arrayBuffer();})]);
        const player=new RolMusic(rol,bank),events=player.compile(this.context.sampleRate);
        if(playbackId!==this.musicGeneration)return false;
        this.musicActive=true;this.musicPlaybackId=playbackId;this.resyncNeeded=true;
        this.node.port.postMessage({type:'music-start',events,durationFrames:player.durationFrames,loopFrames:loop?player.durationFrames:0,playbackId});
        return true;
    }
    stopMusic(){
        ++this.musicGeneration;this.musicPlaybackId=0;
        if(!this.musicActive)return;this.musicActive=false;this.resyncNeeded=true;
        if(this.ready)this.node.port.postMessage({type:'music-stop'});
    }
    async close(){
        this.stopMusic();
        if(!this.context)return;
        this.node?.disconnect();await this.context.close();this.node=null;this.context=null;this.ready=false;this.musicActive=false;this.resyncNeeded=true;
    }
}
