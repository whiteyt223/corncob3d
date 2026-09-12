// Browser hardware boundary for the original two-axis/two-button game port.
// Physical stick selection is a host preference; this class never enables the
// source joystick flag or adds controls absent from the original joystick.
export const JOYSTICK_TEXT=Object.freeze({
  heading:'Calibrating joystick.  Hit esc to exit program.  ',
  stages:[
    'Move joystick to upper left corner, then press button',
    'Move joystick to lower rght corner, then press button',
    'Let joystick flop to the center, then press button   ',
  ],
  retry:'Bad joystick value, please try again',
  abort:'Escape detected, aborting program.\r\n\n',
});

export function gamepadAxisToCounter(axis){
  if(!Number.isFinite(axis))return null;
  // Explicit modern-device boundary: full axis travel becomes timing counts
  // 1..1025. The original calibration and SPKE run after this conversion.
  return 1+Math.round((Math.max(-1,Math.min(1,axis))+1)*512);
}
export function gamepadButtonPort(buttons){
  // Return released axis bits (not hardware timing), with active-low buttons.
  let port=0x30;
  if(buttons?.[0]?.pressed)port&=~0x10;
  if(buttons?.[1]?.pressed)port&=~0x20;
  return port;
}

export class GamepadJoystick {
  constructor({getGamepads=()=>globalThis.navigator?.getGamepads?.()??[],index=null}={}){
    this.getGamepads=getGamepads;this.index=index;
  }
  select(index){
    if(index!==null&&(!Number.isInteger(index)||index<0))throw new RangeError('Gamepad index must be nonnegative or null');
    this.index=index;
  }
  devices(){
    let pads;try{pads=Array.from(this.getGamepads()??[]);this.error=null;}
    catch(error){this.error=error.message??String(error);return [];}
    return pads.filter(p=>p&&p.connected!==false&&p.axes?.length>=2)
      .map(p=>({index:p.index,id:p.id,mapping:p.mapping??''}));
  }
  sample(){
    let pads;try{pads=Array.from(this.getGamepads()??[]);this.error=null;}
    catch(error){this.error=error.message??String(error);return {connected:false,port:0x30,rawX:null,rawY:null,error:this.error};}
    const pad=this.index===null?pads.find(p=>p&&p.connected!==false&&p.axes?.length>=2):pads.find(p=>p?.index===this.index);
    if(!pad||pad.connected===false||pad.axes?.length<2)return {connected:false,port:0x30,rawX:null,rawY:null};
    const rawX=gamepadAxisToCounter(pad.axes[0]),rawY=gamepadAxisToCounter(pad.axes[1]);
    if(rawX===null||rawY===null)return {connected:false,port:0x30,rawX:null,rawY:null};
    return {connected:true,index:pad.index,rawX,rawY,port:gamepadButtonPort(pad.buttons)};
  }
}

// Numeric core state belongs to flight DS. This class only schedules the
// original presentation/button waits. Supply monotonic milliseconds to tick.
// It does not call requestAnimationFrame, navigator, DOM or browser dialogs.
export class JoystickCalibration {
  constructor(core,{reentry=false}={}){
    this.core=core;this.reentry=reentry;this.keys=[];this.phase='idle';this.stage=0;
    this.until=0;this.message='';this.result=null;
  }
  begin(){
    this.core.cc_joystick_calibration_begin();this.stage=0;this.result=null;
    this.showStage();return this.snapshot();
  }
  key(code){if(Number.isInteger(code)&&code>=0)this.keys.push(code&255);}
  showStage(){this.phase='release';this.message=JOYSTICK_TEXT.stages[this.stage];}
  finish(code){
    if(this.reentry)this.core.cc_joystick_calibration_finish_reentry();
    this.result=code;this.phase=code===93?'aborted':'complete';
  }
  cancel(){
    // Browser-only recovery for lost/blocked devices or destroyed UI. No raw
    // joystick reading is fabricated. Apply the same abort/reentry cleanup.
    if(['complete','aborted'].includes(this.phase))return this.snapshot();
    this.core.cc_joystick_calibration_abort();this.message=JOYSTICK_TEXT.abort;
    this.finish(93);return {...this.snapshot(),hostCancelled:true};
  }
  snapshot(){
    return {phase:this.phase,stage:this.stage,heading:JOYSTICK_TEXT.heading,message:this.message,
      result:this.result,remainingKeys:[...this.keys],until:this.until};
  }
  tick(now,sample){
    if(!Number.isFinite(now))throw new TypeError('Calibration clock must be finite');
    if(this.phase==='between'&&now>=this.until)this.showStage();
    if(this.phase==='retry-delay'&&now>=this.until){
      this.message=JOYSTICK_TEXT.retry;this.phase='retry-message';this.until=now+2000;
    }else if(this.phase==='retry-message'&&now>=this.until){this.stage=0;this.showStage();}
    if(!sample?.connected)return this.snapshot();
    if(this.phase==='release'){
      if((sample.port&0x30)===0x30){this.phase='debounce';this.until=now+80;}
      return this.snapshot();
    }
    if(this.phase==='debounce'){
      if(now<this.until)return this.snapshot();
      this.phase='press';
    }
    if(this.phase!=='press')return this.snapshot();
    // JOYBTN first checks the keyboard at each armed poll. First two stages
    // consume a character; the center stage ignores carry and leaves it queued.
    if(this.keys.length){
      if(this.stage<2&&this.keys.shift()===27){
        this.core.cc_joystick_calibration_abort();this.message=JOYSTICK_TEXT.abort;
        this.finish(93);return this.snapshot();
      }
    }else if((sample.port&0x30)===0x30)return this.snapshot();
    const result=this.core.cc_joystick_calibration_sample(this.stage,sample.rawX,sample.rawY)>>>0;
    if(result===0){this.message='';this.finish(0);}
    else if(result===1||result===2){this.stage=result;this.phase='between';this.message='';this.until=now+500;}
    else if(result===3){this.phase='retry-delay';this.message='';this.until=now+500;}
    else throw new Error(`Invalid joystick calibration stage ${this.stage}`);
    return this.snapshot();
  }
}

export function joystickAxes(core,sample){
  if(!sample?.connected)return {connected:false};
  const packed=core.cc_joystick_input(sample.rawX,sample.rawY)>>>0;
  if(packed===0xffffffff)return {connected:true,arithmeticFault:true};
  return {connected:true,x:(packed&65535)-512,y:(packed>>>16)-512,port:sample.port};
}
