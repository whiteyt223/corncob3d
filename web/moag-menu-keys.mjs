import {bossScreen,BOSS_RECORD} from './boss.mjs';
// MOAG DC62 input reader; negative results are reader actions, not menu keys.
export const MOAG_NONE=-1,MOAG_BOSS=-2,MOAG_BEEP=-3;
const modifiers=new Set(['Shift','Control','Alt','Meta','CapsLock','NumLock','ScrollLock','Dead','Process','Unidentified','Pause','PrintScreen']);
const navigation={Enter:10,Escape:27,Backspace:8,Tab:9,Space:32,ArrowUp:16,ArrowDown:14,ArrowLeft:2,ArrowRight:6,PageUp:15,PageDown:25,Home:20,End:18,Insert:1,Delete:24};
export function moagPreflightKey(event){
    if(!event.key||modifiers.has(event.key))return MOAG_NONE;
    const code=event.code.startsWith('Numpad')&&event.key in navigation?event.key:event.code;
    if(/^F\d+$/.test(code))return !event.shiftKey&&!event.ctrlKey&&!event.altKey&&!event.metaKey?(code==='F1'?MOAG_BOSS:code==='F10'?72:MOAG_BEEP):MOAG_BEEP;
    if(event.metaKey)return MOAG_NONE;
    if(event.altKey)return MOAG_BEEP;
    if(event.ctrlKey){
        if(code==='ArrowLeft')return 115;if(code==='ArrowRight')return 116;
        if(code==='Backspace')return 127;
        if(code in navigation&&code!=='Enter'&&code!=='Escape')return MOAG_BEEP;
        if(event.key.length===1){const c=event.key.toUpperCase().charCodeAt(0);if(c>=64&&c<=95){const ch=c&31;return ch===0?MOAG_BEEP:ch===13?10:ch;}}
    }
    if(code in navigation)return navigation[code];
    if(event.key==='Clear')return MOAG_BEEP;
    if(event.key.length!==1)return MOAG_NONE;const ch=event.key.charCodeAt(0)&255;return ch===0?MOAG_BEEP:ch===13?10:ch;
}
export function installMoagMenuKeys(menu){
    const screen=menu.screen,key=menu.key;let saved=null;
    menu.screen=function(...args){this.preflightKeys=false;return screen.apply(this,args);};
    const restore=()=>{
        const old=saved;saved=null;menu.root.replaceChildren(...old.nodes);menu.content=old.content;menu.message=old.message;menu.keyHandler=old.keyHandler;menu.preflightKeys=old.preflightKeys;
        if(old.screen===null)menu.root.removeAttribute('data-screen');else menu.root.setAttribute('data-screen',old.screen);
        menu.backAction=old.backAction;
        if(old.boss===null)menu.root.removeAttribute('data-boss');else menu.root.setAttribute('data-boss',old.boss);
        old.focus?.focus?.();
    };
    const boss=()=>{
        saved={focus:document.activeElement,nodes:[...menu.root.childNodes],content:menu.content,message:menu.message,keyHandler:menu.keyHandler,preflightKeys:menu.preflightKeys,boss:menu.root.getAttribute('data-boss'),screen:menu.root.getAttribute('data-screen'),backAction:menu.backAction};
        menu.screen('');menu.root.setAttribute('data-boss','');menu.content.append(bossScreen(menu.game.career.records.get(BOSS_RECORD)));menu.nav([['Return',restore]]);
        menu.keyHandler=event=>{if(modifiers.has(event.key))return;event.preventDefault();restore();};
    };
    menu.key=function(event){
        if(event.target.tagName==='INPUT'||event.target.tagName==='TEXTAREA')return key.call(this,event);
        if(saved){if(!modifiers.has(event.key)){event.preventDefault();restore();}return;}
        if(this.busy)return;
        if(this.preflightKeys){
            const value=moagPreflightKey(event);if(value===MOAG_NONE)return;
            if(value===MOAG_BOSS){event.preventDefault();boss();return;}
            if(value===MOAG_BEEP){event.preventDefault();void this.run(()=>this.actions.sourceBeep?.());return;}
            const code=event.code.startsWith('Numpad')&&event.key in navigation?event.key:event.code;
            return key.call(this,{code,key:event.key,moagKey:value,repeat:false,target:event.target,preventDefault:()=>event.preventDefault()});
        }
        // Menus with no flight session also use MOAG's F1 boss shortcut.
        // Inflight tower/sourceNotice and editable fields keep their readers.
        if(!this.game.session&&event.code==='F1'&&!event.shiftKey&&!event.ctrlKey&&!event.altKey&&!event.metaKey){event.preventDefault();boss();return;}
        return key.call(this,event);
    };
}
