/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * ROL/BNK event player derived from AdPlug's CrolPlayer and
 * CcomposerBackend (rol.cpp, composer.cpp; Copyright 1999-2008 Simon Peter,
 * OPLx, Stas'M, Jepael).  This module produces YM3812 register writes only;
 * YMFM remains the sole audio synthesizer.
 *
 * The derivation is deliberately separate from game code so that its complete
 * corresponding source and the LGPL notice can ship with the browser module.
 */
const OP = [0,1,2,8,9,10,16,17,18];
const DRUM_OP = [20,18,21,17];
const FNUM = [
[343,364,385,408,433,459,486,515,546,579,614,650],
[344,365,387,410,434,460,488,517,548,581,615,652],
[345,365,387,410,435,461,489,518,549,582,617,653],
[346,366,388,411,436,462,490,519,550,583,618,655],
[346,367,389,412,437,463,491,520,551,584,619,657],
[347,368,390,413,438,464,492,522,553,586,621,658],
[348,369,391,415,439,466,493,523,554,587,622,660],
[349,370,392,415,440,467,495,524,556,589,624,661],
[350,371,393,416,441,468,496,525,557,590,625,663],
[351,372,394,417,442,469,497,527,558,592,627,665],
[351,372,395,418,443,470,498,528,559,593,628,666],
[352,373,396,419,444,471,499,529,561,594,630,668],
[353,374,397,420,445,472,500,530,562,596,631,669],
[354,375,398,421,447,473,502,532,564,597,633,671],
[355,376,398,422,448,474,503,533,565,599,634,672],
[356,377,399,423,449,475,504,534,566,600,636,674],
[356,378,400,424,450,477,505,535,567,601,637,675],
[357,379,401,425,451,478,506,537,569,603,639,677],
[358,379,402,426,452,479,507,538,570,604,640,679],
[359,380,403,427,453,480,509,539,571,606,642,680],
[360,381,404,428,454,481,510,540,572,607,643,682],
[360,382,405,429,455,482,511,541,574,608,645,683],
[361,383,406,430,456,483,512,543,575,610,646,685],
[362,384,407,431,457,484,513,544,577,611,648,687],
[363,385,408,432,458,485,514,545,578,612,649,688]
];
const I16=n=>n>0x7fff?n-0x10000:n;
class Reader {
  constructor(bytes){this.bytes=bytes instanceof Uint8Array?bytes:new Uint8Array(bytes);this.view=new DataView(this.bytes.buffer,this.bytes.byteOffset,this.bytes.byteLength);this.p=0;}
  need(n){if(this.p+n>this.bytes.length)throw new Error('truncated ROL/BNK');}
  u8(){this.need(1);return this.bytes[this.p++];} u16(){this.need(2);const n=this.view.getUint16(this.p,true);this.p+=2;return n;}
  i16(){return I16(this.u16());} i32(){this.need(4);const n=this.view.getInt32(this.p,true);this.p+=4;return n;}
  f32(){this.need(4);const n=this.view.getFloat32(this.p,true);this.p+=4;return n;}
  skip(n){this.need(n);this.p+=n;} seek(n){if(n<0||n>this.bytes.length)throw new Error('invalid ROL/BNK offset');this.p=n;}
  text(n){this.need(n);let s='';for(let i=0;i<n;i++){const c=this.bytes[this.p++];if(c)s+=String.fromCharCode(c);}return s;}
  name8(){this.need(9);let s='',ended=false;for(let i=0;i<8;i++){const c=this.bytes[this.p++];if(c&&!ended)s+=String.fromCharCode(c);else if(!c)ended=true;}this.p++;return s;}
}
function ci(s){return s.toUpperCase();}
function opFrom(r){const k=r.u8(),mul=r.u8(),feedback=r.u8(),attack=r.u8(),sustain=r.u8(),sustainSound=r.u8(),decay=r.u8(),release=r.u8(),level=r.u8(),am=r.u8(),vib=r.u8(),env=r.u8(),fm=r.u8();return {ammulti:(am<<7)|(vib<<6)|(sustainSound<<5)|(env<<4)|mul,ksltl:(k<<6)|level,ardr:(attack<<4)|decay,slrr:(sustain<<4)|release,fbc:(feedback<<1)|(fm^1)};}
function parseBank(bytes){
  const r=new Reader(bytes);r.u8();r.u8();if(r.text(6)!=='ADLIB-')throw new Error('not an ADLIB BNK');r.u16();const total=r.u16(),names=r.i32(),data=r.i32();r.seek(names);const byName=new Map();
  for(let i=0;i<total;i++){const index=r.u16(),used=r.u8(),name=r.name8();if(used)byName.set(ci(name),index);}
  return name=>{const index=byName.get(ci(name));if(index===undefined)return {mod:{ammulti:0,ksltl:0,ardr:0,slrr:0,fbc:0,wave:0},car:{ammulti:0,ksltl:0,ardr:0,slrr:0,fbc:0,wave:0}};const q=new Reader(bytes);q.seek(data+index*30);q.u8();q.u8();const mod=opFrom(q),car=opFrom(q);mod.wave=q.u8();car.wave=q.u8();return {mod,car};};
}
function parseRol(rol,bank){
  let eventCount=0;const budget=n=>{eventCount+=n;if(eventCount>100000)throw new Error('ROL exceeds 100,000 events');return n;};
  const r=new Reader(rol),major=r.u16(),minor=r.u16();if(major!==0||minor!==4)throw new Error(`unsupported ROL ${major}.${minor}`);r.text(40);const tpb=r.u16();r.u16();r.u16();r.u16();r.skip(1);const mode=r.u8();r.skip(90+38+15);const tempo=r.f32();const tempos=[];for(let i=0,n=r.u16();i<n;i++)tempos.push({time:r.i16(),mul:r.f32()});
  budget(tempos.length);
  if(mode>1||!tpb||!Number.isFinite(tempo)||tempo<=0||tempos.some(x=>!Number.isFinite(x.mul)||x.mul<=0))throw new Error('invalid ROL tempo or mode');
  let last=0;const voices=[];for(let voice=0,n=mode?9:11;voice<n;voice++){
    r.skip(15);const end=r.i16(),notes=[];if(end<0)throw new Error('invalid ROL end time');let duration=0;if(end){do{const number=r.i16(),d=r.i16();if(d<=0)throw new Error('invalid ROL note duration');budget(1);notes.push({number,duration:d});duration+=d;}while(duration<end);last=Math.max(last,end);}r.skip(15);
    const instruments=[];for(let i=0,n=budget(r.u16());i<n;i++){const time=r.i16(),name=r.name8();r.skip(3);instruments.push({time,name,instrument:bank(name)});}r.skip(15);
    const volumes=[];for(let i=0,n=budget(r.u16());i<n;i++){const time=r.i16(),mul=r.f32();if(!Number.isFinite(mul))throw new Error('invalid ROL volume');volumes.push({time,mul});}r.skip(15);
    const pitches=[];for(let i=0,n=budget(r.u16());i<n;i++){const time=r.i16(),variation=r.f32();if(!Number.isFinite(variation))throw new Error('invalid ROL pitch');pitches.push({time,variation});}
    voices.push({notes,instruments,volumes,pitches});
  }
  return {ticksPerBeat:tpb,mode,tempo,tempos,voices,last,eventCount};
}
export class RolMusic {
  constructor(rolBytes,bnkBytes){this.song=parseRol(rolBytes,parseBank(bnkBytes));this.writes=[];this.rewind();}
  write(reg,value){this.writes.push([reg&255,value&255]);}
  drain(){const out=this.writes;this.writes=[];return out;}
  refresh(){return (Math.min(60,this.song.ticksPerBeat)*this.song.tempo*this.tempoMultiplier)/60;}
  rewind(){
    this.tick=0;this.tempoIndex=0;this.tempoMultiplier=1;this.half=new Int16Array(11);this.vol=new Uint8Array(11).fill(127);this.ksl=new Uint8Array(11);this.note=new Int16Array(11);this.kon=new Uint8Array(11);this.high=new Uint8Array(11);this.fnumRow=new Int16Array(11);this.oldBend=0x7fffffff;this.oldRow=0;this.oldHalf=0;this.rhythm=0;this.rhythmReg=0;this.state=this.song.voices.map(()=>({note:0,noteDuration:0,noteAge:0,instrument:0,volume:0,pitch:0,force:true,ended:false,instEnd:false,volumeEnd:false,pitchEnd:false}));
    this.write(1,0x20);this.setRhythm(this.song.mode^1);
  }
  setRhythm(on){if(on){this.rhythmReg|=0x20;this.write(0xbd,this.rhythmReg);this.setFreq(8,24);this.setFreq(7,31);}else{this.rhythmReg&=~0x20;this.write(0xbd,this.rhythmReg);}this.rhythm=on?1:0;}
  setFreq(voice,note,keyOn=false){const n=Math.max(0,Math.min(95,note+this.half[voice]));const f=FNUM[this.fnumRow[voice]][n%12];this.note[voice]=note;this.kon[voice]=keyOn?1:0;const hi=((Math.floor(n/12)<<2)|((f>>8)&3));this.high[voice]=hi;if(voice<9)this.write(0xa0+voice,f);if(voice<9)this.write(0xb0+voice,hi|(keyOn?0x20:0));}
  setNote(voice,note){if(voice<6||!this.rhythm)this.setNoteMelodic(voice,note);else this.setNoteDrum(voice,note);}
  setNoteMelodic(voice,note){if(voice>=9)return;this.write(0xb0+voice,this.high[voice]&~0x20);this.kon[voice]=0;if(note!==-12)this.setFreq(voice,note,true);}
  setNoteDrum(voice,note){const mask=1<<(10-voice);this.rhythmReg&=~mask;this.write(0xbd,this.rhythmReg);this.kon[voice]=0;if(note!==-12){if(voice===8){this.setFreq(8,note);this.setFreq(7,note+7);}else if(voice===6)this.setFreq(voice,note);this.kon[voice]=1;this.rhythmReg|=mask;this.write(0xbd,this.rhythmReg);}}
  noteOn(voice,note){this.setNote(voice,note-12);} noteOff(voice){this.setNote(voice,-12);}
  carrierOffset(voice){return voice<7||!this.rhythm?OP[voice]+3:DRUM_OP[voice-7];}
  level(voice){let n=63-(this.ksl[voice]&63);n=this.vol[voice]*n;n+=n+127;n=63-Math.floor(n/(2*127));return n|(this.ksl[voice]&0xc0);}
  setVolume(voice,volume){if(voice>=9&&!this.rhythm)return;this.vol[voice]=volume;this.write(0x40+this.carrierOffset(voice),this.level(voice));}
  sendInstrument(voice,i){
    if(voice>=9&&!this.rhythm)return;const m=i.mod,c=i.car;
    if(voice<7||!this.rhythm){const o=OP[voice];this.write(0x20+o,m.ammulti);this.write(0x40+o,m.ksltl);this.write(0x60+o,m.ardr);this.write(0x80+o,m.slrr);this.write(0xc0+voice,m.fbc);this.write(0xe0+o,m.wave);this.ksl[voice]=c.ksltl;this.write(0x20+o+3,c.ammulti);this.write(0x40+o+3,this.level(voice));this.write(0x60+o+3,c.ardr);this.write(0x80+o+3,c.slrr);this.write(0xe0+o+3,c.wave);
    }else{const o=DRUM_OP[voice-7];this.ksl[voice]=m.ksltl;this.write(0x20+o,m.ammulti);this.write(0x40+o,this.level(voice));this.write(0x60+o,m.ardr);this.write(0x80+o,m.slrr);this.write(0xe0+o,m.wave);}
  }
  setPitch(voice,variation){if(voice>=6&&this.rhythm)return;const bend=variation===1?0x2000:Math.trunc(0x1fff*variation);const length=(bend-0x2000)*25;let row,half;if(this.oldBend===length){row=this.oldRow;half=this.oldHalf;}else{// C++ divides signed pitchBendLength by unsigned kMidPitch, then narrows to int16.
    const quotient=Math.floor((length>>>0)/0x2000);const d16=quotient&0xffff;const dir=d16&0x8000?d16-0x10000:d16;let delta;if(dir<0){const down=24-dir;half=-Math.trunc(down/25);delta=(down-24)%25;if(delta)delta=25-delta;}else{half=Math.trunc(dir/25);delta=dir%25;}row=delta;this.oldBend=length;this.oldRow=row;this.oldHalf=half;}this.fnumRow[voice]=row;this.half[voice]=half;this.setFreq(voice,this.note[voice],!!this.kon[voice]);}
  updateVoice(voice,data,s){
    if(!data.notes.length||s.ended)return;
    if(!s.instEnd){if(s.instrument<data.instruments.length){const x=data.instruments[s.instrument];if(x.time===this.tick){this.sendInstrument(voice,x.instrument);s.instrument++;}}else s.instEnd=true;}
    if(!s.volumeEnd){if(s.volume<data.volumes.length){const x=data.volumes[s.volume];if(x.time===this.tick){this.setVolume(voice,Math.trunc(127*x.mul));s.volume++;}}else s.volumeEnd=true;}
    if(s.force||s.noteAge>s.noteDuration-1){if(this.tick!==0)s.note++;if(s.note<data.notes.length){const x=data.notes[s.note];this.noteOn(voice,x.number);s.noteAge=0;s.noteDuration=x.duration;s.force=false;}else{this.noteOff(voice);s.ended=true;return;}}
    if(!s.pitchEnd){if(s.pitch<data.pitches.length){const x=data.pitches[s.pitch];if(x.time===this.tick){this.setPitch(voice,x.variation);s.pitch++;}}else s.pitchEnd=true;}
    s.noteAge++;
  }
  update(){if(this.tempoIndex<this.song.tempos.length&&this.song.tempos[this.tempoIndex].time===this.tick){this.tempoMultiplier=this.song.tempos[this.tempoIndex++].mul;}for(let v=0;v<this.song.voices.length;v++)this.updateVoice(v,this.song.voices[v],this.state[v]);this.tick++;return this.tick<=this.song.last;}
  /* Return [sampleOffset, register, value] for one non-looping performance. */
  compile(sampleRate){if(!Number.isFinite(sampleRate)||sampleRate<8000||sampleRate>384000)throw new Error('invalid music sample rate');this.writes=[];this.rewind();const events=[];let sample=0;for(const [r,v] of this.drain())events.push([0,r,v]);for(;;){const at=Math.round(sample);const more=this.update();const hz=this.refresh();for(const [r,v] of this.drain())events.push([at,r,v]);sample+=sampleRate/hz;if(!Number.isFinite(sample)||sample>sampleRate*3600)throw new Error('ROL playback exceeds one hour');if(!more)break;}this.durationFrames=Math.round(sample);return events;}
}
