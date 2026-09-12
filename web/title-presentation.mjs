// Original CCTITLE navigation with exact source-rendered cards and one original
// animation loop per supplied clock event. The original animation has no timer;
// only story fades use a fixed25ms delay.
export function titleRGBA(indices,palette,step=16){
 if(indices.length!==64000||palette.length!==768)throw Error('The original title frame is incomplete.');
 const rgba=new Uint8ClampedArray(256000);
 for(let i=0;i<64000;i++){
  for(let c=0;c<3;c++){const source=palette[indices[i]*3+c],v=(step===16?source:Math.floor(source*step/16))&63;rgba[i*4+c]=(v<<2)|(v>>4);}
  rgba[i*4+3]=255;
 }return rgba;
}
export function titleKeyBytes(event){
 const fixed={Escape:27,Enter:13,Tab:9,Backspace:8};if(event.key in fixed)return[fixed[event.key]];
 if(event.key?.length===1){const c=event.key.codePointAt(0);return[c<256?c:63];}
 const scan={ArrowUp:72,ArrowDown:80,ArrowLeft:75,ArrowRight:77,Home:71,End:79,PageUp:73,PageDown:81,Insert:82,Delete:83,F1:59,F2:60,F3:61,F4:62,F5:63,F6:64,F7:65,F8:66,F9:67,F10:68,F11:133,F12:134};
 return event.key in scan?[0,scan[event.key]]:[];
}
export function titleAudioEvents(e){
 const view=new DataView(e.memory.buffer,e.cc_title_events(),e.cc_title_event_count()*8),events=[];
 for(let i=0;i<view.byteLength;i+=8)events.push({routine:view.getUint16(i,true),a:view.getUint16(i+2,true),b:view.getUint16(i+4,true)});return events;
}
export class TitlePresentation {
 constructor({canvas,exports:e,manifest,baseURL='presentation/',events=globalThis.window,
  read=async url=>{const r=await fetch(url);if(!r.ok)throw Error(`${url}: HTTP ${r.status}`);return new Uint8Array(await r.arrayBuffer());},
  paint=null,delay=ms=>new Promise(resolve=>setTimeout(resolve,ms)),nextFrame=()=>new Promise(resolve=>requestAnimationFrame(resolve)),
  onScene=()=>{},onActive=()=>{},onAudio=()=>{},onAudioBatch=()=>{},musicPlaying=()=>false}){
  Object.assign(this,{canvas,e,manifest,baseURL,events,read,delay,nextFrame,onScene,onActive,onAudio,onAudioBatch,musicPlaying});
  this.paint=paint??(rgba=>canvas.getContext('2d',{alpha:false}).putImageData(new ImageData(rgba,320,200),0,0));
  this.active=false;this.queue=[];this.waiter=null;this.prepared=null;
  this.keydown=event=>{if(!this.active)return;event.preventDefault();event.stopImmediatePropagation();for(const key of titleKeyBytes(event))this.input(key);};
  this.keyup=event=>{if(this.active){event.preventDefault();event.stopImmediatePropagation();}};
  events.addEventListener('keydown',this.keydown,true);events.addEventListener('keyup',this.keyup,true);
 }
 input(byte){if(!this.active)return;this.queue.push(byte&255);if(this.waiter){const r=this.waiter;this.waiter=null;r(this.queue.shift());}}
 skip(){this.input(27);}
 async key(){if(this.queue.length)return this.queue.shift();return new Promise(resolve=>{this.waiter=resolve;});}
 async prepare(){
  if(!this.prepared)this.prepared=Promise.all([this.read(this.baseURL+this.manifest.data.file),...this.manifest.cards.map(c=>this.read(this.baseURL+c.frame))]).then(([data,...cards])=>{
   if(data.length!==this.manifest.data.bytes||data.length>this.e.cc_title_data_capacity())throw Error('The original title data is incomplete.');
   for(const card of cards)if(card.length!==64768)throw Error('The original story card is incomplete.');return{data,cards};
  }).catch(error=>{this.prepared=null;throw error;});return this.prepared;
 }
 drawCard(bytes,step){this.paint(titleRGBA(bytes.subarray(0,64000),bytes.subarray(64000),step));}
 async fade(bytes,direction){
  for(let i=0;i<16;i++){this.drawCard(bytes,direction==='in'?i:15-i);await this.delay(25);if(this.queue.length)return false;}
  if(direction==='in')this.drawCard(bytes,16);return true;
 }
 drawAnimation(){const e=this.e;this.paint(titleRGBA(new Uint8Array(e.memory.buffer,e.cc_title_pixels(),64000),new Uint8Array(e.memory.buffer,e.cc_title_palette(),768)));}
 async audio(){for(const event of titleAudioEvents(this.e))await this.onAudio(event);await this.onAudioBatch();}
 async show({music=false,sound=true,seed=1,initialKey=-1}={}){
  if(this.active)throw Error('The title is already active.');
  // Original image38FC consumes an already-queued key and exits before graphics.
  if(initialKey>=0)return{reason:'initial-key',remainingKeys:[]};
  const saved={width:this.canvas.width,height:this.canvas.height,aspectRatio:this.canvas.style.aspectRatio,label:this.canvas.getAttribute('aria-label')};
  this.active=true;this.queue=[];this.onActive(true);let reason='escape';
  try{
   const {data,cards}=await this.prepare();this.canvas.width=320;this.canvas.height=200;this.canvas.style.aspectRatio='4 / 3';this.canvas.focus();
   // Original image44D2 checks keys once after loading: any key skips the story
   // cards; Escape exits the complete title.
   let skipCards=false;if(this.queue.length){skipCards=true;if(this.queue.shift()===27)return{reason:'load-escape',remainingKeys:this.queue.slice()};}
   if(!skipCards)for(let i=0;i<cards.length;i++){
    const scene=this.manifest.cards[i];this.canvas.setAttribute('aria-label',scene.text.map(t=>t.text).join(' '));this.onScene({kind:'story',scene,index:i});
    const faded=await this.fade(cards[i],'in');const key=await this.key();
    if(!faded||key===27)return{reason:!faded?'fade-key':'story-escape',remainingKeys:this.queue.slice()};
    if(!await this.fade(cards[i],'out'))return{reason:'fade-key',remainingKeys:this.queue.slice()};
   }
   const e=this.e;new Uint8Array(e.memory.buffer,e.cc_title_data(),data.length).set(data);e.cc_title_init_profile(this.manifest.profile??0,Number(music),Number(sound),seed>>>0);await this.audio();
   let phase=-1;
   while(e.cc_title_phase()!==2){
    const current=e.cc_title_phase();if(current!==phase){phase=current;this.canvas.setAttribute('aria-label',current===0?'Corncob aircraft title animation':'Corncob title and credits');this.onScene({kind:'animation',phase:current});}
    this.drawAnimation();await this.nextFrame();e.cc_title_step(this.queue.length?this.queue.shift():-1,Number(this.musicPlaying()));await this.audio();
   }
   return{reason,remainingKeys:this.queue.slice()};
  }finally{
   this.active=false;this.waiter=null;this.queue=[];this.canvas.width=saved.width;this.canvas.height=saved.height;this.canvas.style.aspectRatio=saved.aspectRatio;
   if(saved.label===null)this.canvas.removeAttribute('aria-label');else this.canvas.setAttribute('aria-label',saved.label);this.onActive(false);
  }
 }
 dispose(){if(this.active)throw Error('Finish the title before disposing.');this.events.removeEventListener('keydown',this.keydown,true);this.events.removeEventListener('keyup',this.keyup,true);}
}

// Expand source SOADL wrapper events through the existing source OPL module.
// The music owner handles the two returned song events; all other events are
// synchronous so the register sequence retains original order.
export function titleAudioToCore(e,event){
 const {routine,a,b}=event;
 if(routine===0x447)e.cc_audio_write(a&255,b&255);
 else if(routine===0x3d0)e.cc_audio_voice_on(a,b);
 else if(routine===0x3f9)e.cc_audio_write((0xb0+a)&255,(((b>>>8)<<2)|1)&0xdf);
 else if(routine===0x422){e.cc_audio_register_init();e.cc_audio_init_voices();e.cc_audio_preset(9);}
 else if(routine===0x5bfb)return'start-title-music';
 else if(routine===0x5c65)return'stop-title-music';
 else throw Error(`Unknown original title audio routine ${routine.toString(16)}`);
 return null;
}
