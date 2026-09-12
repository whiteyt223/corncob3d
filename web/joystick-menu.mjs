import {JoystickCalibration} from './gamepad-joystick.mjs';
const node=(tag,text)=>{const n=document.createElement(tag);if(text!==undefined)n.textContent=text;return n;};
// The persistent words are the existing 28-byte Career metadata fields 2/4.
// Force-recenter and browser device identity are not original saved fields.
export class JoystickSettings {
  constructor(career){this.career=career;this.force=0;}
  words(){const d=new DataView(this.career.metadata.buffer,this.career.metadata.byteOffset);return [d.getUint16(2,true),this.force,d.getUint16(4,true)];}
  get keyboard(){return !!this.words()[0];}
  get manual(){return !!this.words()[2];}
  async choose(kind){
    const force=await this.career.setControls(kind,this.force);this.force=force;
  }
  flags(){
    const e=this.career.e,p=e.cc_browser_workspace();new Uint16Array(e.memory.buffer,p,3).set(this.words());
    return e.cc_menu_controls_flags(p,0);
  }
}
export function joystickOptions(menu,settings,hardware){
  const box=menu.screen('Input device'),devices=hardware.devices();
  box.append(node('p',settings.keyboard?'Keyboard controls selected.':'Joystick controls selected.'));
  box.append(node('p','Joystick: roll and pitch; button 1 fires bullets; button 2 or Space fires missiles. Rudder Z/X and throttle +/− remain keyboard controls. Left Shift also applies brakes.'));
  const label=node('label','Device '),select=node('select');
  for(const device of devices){const option=node('option',device.id||`Joystick ${device.index+1}`);option.value=String(device.index);select.append(option);}
  if(hardware.index!==null)select.value=String(hardware.index);
  select.disabled=!devices.length;select.addEventListener('change',()=>hardware.select(Number(select.value)));
  if(devices.length&&hardware.index===null)hardware.select(devices[0].index);
  label.append(select);box.append(label);
  if(!devices.length)box.append(node('p',hardware.error?`Gamepad access is unavailable: ${hardware.error}`:'Connect a joystick or gamepad and press a button, then refresh devices. Keyboard controls remain available.'));
  const choose=async kind=>{if(kind===1&&!hardware.sample().connected)throw Error('Connect a joystick and refresh devices first.');await settings.choose(kind);joystickOptions(menu,settings,hardware);};
  menu.nav([['Keyboard',()=>choose(0)],['Joystick',()=>choose(1)],['Recenter on next launch',()=>choose(2)],
    [settings.manual?'Automatic centering':'Manual centering',()=>choose(settings.manual?3:4)],
    ['Refresh devices',()=>joystickOptions(menu,settings,hardware)],['Controls',()=>menu.controls()]]);
}
function character(event){
  if(event.code==='Escape')return 27;if(event.code==='Enter')return 13;if(event.code==='Space')return 32;
  if(event.key?.length===1)return event.key.charCodeAt(0)&255;
  if(/^Key[A-Z]$/.test(event.code))return event.code.charCodeAt(3);
  if(/^Digit[0-9]$/.test(event.code))return event.code.charCodeAt(5);
  return null;
}
const calibrationFrame=()=>new Promise(resolve=>{
  const frame=now=>{
    // A physical gamepad can remain active while another window owns input.
    // Completing this modal resumes or starts flight, so wait for focus.
    if(document.hidden||!document.hasFocus()){requestAnimationFrame(frame);return;}
    resolve(now);
  };
  requestAnimationFrame(frame);
});
export async function calibrateJoystick(menu,e,hardware,{reentry=false,signal,nextFrame=calibrationFrame}={}){
  const calibration=new JoystickCalibration(e,{reentry}),box=menu.screen('Joystick calibration'),heading=node('p'),prompt=node('p'),status=node('p'),recover=node('button','Cancel calibration');
  recover.type='button';recover.hidden=true;box.append(heading,prompt,status,recover);
  let cancelled=false;const cancel=()=>{cancelled=true;calibration.cancel();};
  recover.addEventListener('click',cancel);signal?.addEventListener('abort',cancel,{once:true});
  menu.keyHandler=event=>{if(event.repeat)return;const code=character(event);if(code!==null){event.preventDefault();if(!hardware.sample().connected&&code===27)cancel();else calibration.key(code);}};
  calibration.begin();
  try{
    while(calibration.result===null){
      if(signal?.aborted){cancel();break;}
      const now=await nextFrame();if(cancelled)break;
      const sample=hardware.sample(),view=calibration.tick(now,sample);
      heading.textContent=view.heading;prompt.textContent=view.message;
      recover.hidden=sample.connected;
      status.textContent=sample.connected?'':sample.error?`Gamepad access is unavailable: ${sample.error}`:'Joystick disconnected. Reconnect it to continue, or cancel calibration.';
    }
    return {...calibration.snapshot(),hostCancelled:cancelled};
  }finally{
    // Exceptions/teardown must clear reentry's diagnostic/request and listeners.
    if(calibration.result===null)calibration.cancel();
    signal?.removeEventListener('abort',cancel);recover.removeEventListener('click',cancel);menu.keyHandler=null;
  }
}
export function joystickRecovery(menu,hardware,{inFlight=false}={}){
  const box=menu.screen('Joystick unavailable');box.append(node('p','Reconnect the selected device, or choose keyboard controls.'));
  // Plain button callbacks deliberately work while the outer launch action is
  // awaiting this modal, when GameMenu.run already owns its busy flag.
  return new Promise(resolve=>{
    const finish=value=>{menu.keyHandler=null;resolve(value);};
    for(const [text,value] of [['Reconnect','retry'],['Use keyboard','keyboard'],[inFlight?'Keep paused':'Cancel launch','cancel']]){
      const b=node('button',text);b.type='button';b.addEventListener('click',()=>{
        if(value==='retry'&&!hardware.sample().connected){menu.message.textContent=hardware.error??'The selected joystick is still unavailable.';return;}
        finish(value);
      });box.append(b);
    }
    menu.keyHandler=event=>{if(event.code==='Escape'){event.preventDefault();finish('cancel');}};
  });
}
