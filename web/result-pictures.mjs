// Original PICT scene continuation. Artwork, captions and DAC come from the
// retained original renderer; this adapter only expands indexed output to RGBA.
export function resultPictureRGBA(bytes){
    if(bytes.length!==64768)throw Error('The original result picture is incomplete.');
    const rgba=new Uint8ClampedArray(320*200*4),palette=bytes.subarray(64000);
    for(let p=0;p<64000;p++){
        const c=bytes[p]*3;
        for(let channel=0;channel<3;channel++){const v=palette[c+channel]&63;rgba[p*4+channel]=(v<<2)|(v>>4);}
        rgba[p*4+3]=255;
    }
    return rgba;
}
export function resultPicturePlan(e,flightDS,engineError=0){
    const at=e.cc_browser_workspace();e.cc_result_picture_plan(flightDS,engineError,at);
    return new Uint8Array(e.memory.buffer,at,8).slice();
}

const NON_TEXT_KEYS=new Set(['ShiftLeft','ShiftRight','ControlLeft','ControlRight','AltLeft','AltRight','MetaLeft','MetaRight','CapsLock','NumLock','ScrollLock']);

export class ResultPictures {
    constructor({canvas,manifest,baseURL='presentation/',events=globalThis.window,
        read=async url=>{const r=await fetch(url);if(!r.ok)throw Error(`${url}: HTTP ${r.status}`);return new Uint8Array(await r.arrayBuffer());},
        paint=null,onScene=()=>{},onActive=()=>{}}){
        this.canvas=canvas;this.manifest=manifest;this.baseURL=baseURL;this.events=events;
        this.read=read;this.paint=paint??((rgba)=>canvas.getContext('2d',{alpha:false}).putImageData(new ImageData(rgba,320,200),0,0));
        this.onScene=onScene;this.onActive=onActive;this.active=false;this.cache=new Map();
        this.down=new Set();this.waiter=null;this.pointer=null;
        this.keydown=event=>{
            const wasDown=this.down.has(event.code);this.down.add(event.code);
            if(!this.active)return;
            event.preventDefault();event.stopImmediatePropagation();
            // Auto-repeat and keys already held at the flight boundary must not
            // satisfy MOAG's post-picture fresh-key wait.
            if(!event.repeat&&!wasDown&&!NON_TEXT_KEYS.has(event.code))this.advance();
        };
        this.keyup=event=>{this.down.delete(event.code);if(this.active){event.preventDefault();event.stopImmediatePropagation();}};
        this.pointerdown=event=>{
            if(!this.active)return;
            event.preventDefault();event.stopImmediatePropagation();this.pointer=event.pointerId;
        };
        this.pointerup=event=>{
            if(!this.active)return;
            event.preventDefault();event.stopImmediatePropagation();
            if(this.pointer===event.pointerId){this.pointer=null;this.advance();}
        };
        events.addEventListener('keydown',this.keydown,true);events.addEventListener('keyup',this.keyup,true);
        canvas.addEventListener('pointerdown',this.pointerdown);canvas.addEventListener('pointerup',this.pointerup);
    }
    advance(){if(this.waiter){const resolve=this.waiter;this.waiter=null;resolve();}}
    wait(){return new Promise(resolve=>{this.waiter=resolve;});}
    async frame(scene){
        if(!this.cache.has(scene.id)){
            const bytes=await this.read(this.baseURL+scene.frame);this.cache.set(scene.id,resultPictureRGBA(bytes));
        }
        return this.cache.get(scene.id);
    }
    async show(plan){
        if(this.active)throw Error('Result pictures are already active.');
        if(plan.length!==8||plan[0]>3)throw Error('The original result-picture plan is invalid.');
        if(!plan[0])return;
        const scenes=[...plan.subarray(4,4+plan[0])].map(id=>{
            const s=this.manifest.scenes.find(s=>s.id===id);if(!s)throw Error('The original result picture is missing.');return s;
        });
        const saved={width:this.canvas.width,height:this.canvas.height,aspectRatio:this.canvas.style.aspectRatio,label:this.canvas.getAttribute('aria-label')};
        this.active=true;this.pointer=null;this.onActive(true);
        try{
            // Download before displaying the first scene. Loading-time keypresses
            // are consumed rather than carried into the first displayed scene.
            const frames=await Promise.all(scenes.map(s=>this.frame(s)));
            this.canvas.width=320;this.canvas.height=200;this.canvas.style.aspectRatio='4 / 3';
            this.canvas.focus();
            for(let i=0;i<scenes.length;i++){
                if(plan[3]&(1<<i))await this.wait();
                this.paint(frames[i]);this.canvas.setAttribute('aria-label',scenes[i].text.map(t=>t.text.trimEnd()).join(' '));
                this.onScene(scenes[i],i,scenes.length);
            }
            // Original MOAG drains queued keys and waits in non-text video mode
            // before reading/evaluating results (image E62F..E66E).
            await this.wait();
        }finally{
            this.waiter=null;this.pointer=null;this.active=false;
            this.canvas.width=saved.width;this.canvas.height=saved.height;this.canvas.style.aspectRatio=saved.aspectRatio;
            if(saved.label===null)this.canvas.removeAttribute('aria-label');else this.canvas.setAttribute('aria-label',saved.label);
            this.onActive(false);
        }
    }
    dispose(){
        if(this.active)throw Error('Wait for the result pictures to finish before disposing.');
        this.events.removeEventListener('keydown',this.keydown,true);this.events.removeEventListener('keyup',this.keyup,true);
        this.canvas.removeEventListener('pointerdown',this.pointerdown);this.canvas.removeEventListener('pointerup',this.pointerup);
    }
}
