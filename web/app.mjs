import {packagedDemoLaunch} from './packaged-demos.mjs';
import {DemoLibrary} from './demo-stream.mjs';
import {demoOptions} from './demo-menu.mjs';
import {GamepadJoystick} from './gamepad-joystick.mjs';
import {JoystickSettings,joystickOptions,calibrateJoystick,joystickRecovery} from './joystick-menu.mjs';
import {FlightInput,PIT_HZ,FRAME_MS,dosClock} from './session.mjs';
import {GameStorage,MemoryGameStorage,SaveWriteError} from './storage.mjs';
import {Game} from './game.mjs';
import {GameMenu} from './menu.mjs';
import {installMoagMenuKeys} from './moag-menu-keys.mjs';
import {ResultPictures,resultPicturePlan} from './result-pictures.mjs';
import {TitlePresentation,titleAudioToCore} from './title-presentation.mjs';
import {PcSpeaker} from './pc-speaker.mjs';
import {PreflightBeep} from './preflight-beep.mjs';
import {AdlibAudio} from './adlib.mjs';
import {RolMusicController} from './rol-music-controller.mjs';
import {SoundEditorController,soundEditorOptions} from './sound-editor.mjs';
import {TheaterInfo} from './theater-info.mjs';
import {loadEditionAssets} from './edition-assets.mjs';
import {createFlight} from './startup.mjs';
import {builderOptions,createBuilderFlight,exportBuilderDefinition,downloadDefinition,builderSaveHandler} from './builder-workspace.mjs';
const $=id=>document.getElementById(id),status=$('status'),canvas=$('frame'),context=canvas.getContext('2d',{alpha:false});
const tick=()=>Math.floor(performance.now()*PIT_HZ/1000);
async function checked(url){const response=await fetch(url);if(!response.ok)throw Error(`${url}: HTTP ${response.status}`);return response;}
const binary=async url=>new Uint8Array(await (await checked(url)).arrayBuffer()),json=async url=>(await checked(url)).json();
try{
    const {e,assets}=await loadEditionAssets({search:location.search,binary,json});
    $('edition-label').textContent=`${assets.edition.label} · Browser port`;
    let storage,storageError;try{storage=await GameStorage.open(assets.editionId);}catch(error){storageError=error.message;storage=new MemoryGameStorage(assets.editionId);}
    const game=new Game(e,assets,storage),input=new FlightInput(tick()),audio=new AdlibAudio(e),music=new RolMusicController(audio,{edition:assets.editionId}),speaker=new PcSpeaker(()=>audio.context);await game.load();
    const sounds=new SoundEditorController(e,{initial:assets.soundAdl,defaults:assets.defaultSoundAdl??assets.soundAdl,read:async name=>game.career.records.get(name)?.slice()??null,write:(_name,bytes)=>game.career.setSoundAdl(bytes)});await sounds.load();
    const joystick=new GamepadJoystick(),joystickSettings=new JoystickSettings(game.career);
    const preflightBeep=new PreflightBeep(speaker),demos=new DemoLibrary(e);
    const consumedResumeKeys=new Set(),capturedFlightKeys=new Set();
    const builderSave=document.createElement('button');builderSave.type='button';builderSave.textContent='Save DEF';builderSave.hidden=true;$('end').before(builderSave);
    let paused=true,last=performance.now(),elapsed=0,finishing=false,audioError,theaterInfo=null;
    const paint=()=>{if(game.session)context.putImageData(new ImageData(game.session.rgba(),640,350),0,0);};
    const say=text=>{status.textContent=text;};
    audio.onMusicEnded=playbackId=>{void music.ended(playbackId).catch(error=>say(`Music is unavailable: ${error.message}`));};
    async function activateAudio(){try{await audio.start();await audio.resume();}catch(error){audioError=error.message;say(`Sound is unavailable: ${audioError}`);}}
    function pause(value,{account=true,preserveSourceKeys=false}={}){
        if(value===paused)return;
        paused=value;input.reset(tick());capturedFlightKeys.clear();if(!preserveSourceKeys)game.session?.releaseKeys();elapsed=0;last=performance.now();
        $('pause').textContent=value?'Resume flight':'Pause';$('pause').setAttribute('aria-pressed',String(value));
        if(value){if(game.session&&!game.session.status&&!game.session.paused&&!game.session.radio&&account){e.cc_sortie_add_elapsed(dosClock());e.cc_audio_pause();}void audio.pause();}
        else{if(account&&!game.session?.paused&&!game.session?.radio)e.cc_sortie_timer_reset(dosClock());void audio.resume();}
        say(value?'Paused':'Flying');
    }
    function joystickCancelled(){const error=Error('Joystick calibration cancelled.');error.joystickCancelled=true;return error;}
    async function keyboardFallback(){
        await joystickSettings.choose(0);
        const ds=new Uint8Array(e.memory.buffer,e.cc_flight_state(),65536);ds[0xaf6]=0;ds[0xae4]=0;
        const rt=new Uint32Array(e.memory.buffer,e.cc_runtime_state(),3);if(rt[1]===91||rt[1]===93)rt[1]=0;
    }
    async function chooseJoystickRecovery(options={}){
        let message;
        for(;;){
            const pending=joystickRecovery(menu,joystick,options);
            if(message)menu.message.textContent=message;
            const choice=await pending;
            if(choice!=='keyboard')return choice;
            try{await keyboardFallback();return choice;}
            catch(error){message=`Could not save keyboard preference: ${error.message}`;}
        }
    }
    async function startupJoystick(){
        for(;;){
            const sample=joystick.sample(),gate=e.cc_joystick_startup(Number(sample.connected),sample.rawX??0,sample.rawY??0);
            if(gate===91){const choice=await chooseJoystickRecovery();if(choice==='retry')continue;if(choice==='keyboard')return;throw joystickCancelled();}
            if(gate===1){const result=await calibrateJoystick(menu,e,joystick);if(result.code===93||result.result===93)throw joystickCancelled();}
            return;
        }
    }
    async function recoverFlightJoystick(){
        const choice=await chooseJoystickRecovery({inFlight:true});
        menu.hide();if(choice==='cancel'){say('Paused');return;}
        pause(false);canvas.focus();
    }
    async function flightCalibration(){
        try{await calibrateJoystick(menu,e,joystick,{reentry:true});}
        finally{game.session.leaveMenu();menu.hide();paint();}
        if(!joystick.sample().connected){await recoverFlightJoystick();return;}
        pause(false,{account:false,preserveSourceKeys:true});canvas.focus();
    }
    const menu=new GameMenu(game,$('menu'),{
        fullscreen:()=>{if(document.fullscreenElement)return document.exitFullscreen();return $('flight-shell').requestFullscreen();},
        sourceBeep:(reader=menu)=>{void activateAudio();return preflightBeep.play(reader);},
        fly:async(extraPlane=false,startAirfield=null)=>{
            const sound=activateAudio();music.leaveMenuForFlight();await game.career.save();try{game.session=await createFlight(e,assets,game.career,{extraPlane,startAirfield,controlFlags:joystickSettings.flags(),joystickStartup:startupJoystick,...demos.launchOptions()});}catch(error){if(error.joystickCancelled){menu.hangar();say('Ready');return;}throw error;}finally{joystickSettings.force=0;}
            menu.hide();$('flight').hidden=false;$('flight-toolbar').hidden=false;audio.reset();await sound;
            paused=true;pause(false);paint();canvas.focus();
        },
        leaveTower:coordinates=>{if(coordinates)game.session.shortcut(coordinates);game.session.leaveMenu();menu.hide();pause(false,{account:false,preserveSourceKeys:true});paint();canvas.focus();},
        resumeMenu:()=>{theaterInfo?.dispose();theaterInfo=null;game.session?.leaveMenu();menu.hide();pause(false,{account:false,preserveSourceKeys:true});canvas.focus();},
        finish:choice=>finish(choice),
        export:async()=>{if(!storage)throw Error(storageError);await game.career.save();const url=URL.createObjectURL(await storage.export()),a=document.createElement('a');a.href=url;a.download='corncob-saves.json';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);},
        joystickOptions:()=>joystickOptions(menu,joystickSettings,joystick),
        demoOptions:()=>demoOptions(menu,demos),
        builder:()=>builderOptions(menu),
        openBuilder:async(bytes,name)=>{
            if(game.session)throw Error('Finish the current sortie before opening a definition.');
            const sound=activateAudio();music.leaveMenuForFlight();
            try{game.session=await createBuilderFlight(e,assets,bytes,{name,controlFlags:joystickSettings.flags(),joystickStartup:startupJoystick});}catch(error){if(error.joystickCancelled){builderOptions(menu);say('Ready');return;}throw error;}finally{joystickSettings.force=0;}
            menu.hide();$('flight').hidden=false;$('flight-toolbar').hidden=false;builderSave.hidden=false;audio.reset();await sound;paused=true;pause(false);paint();canvas.focus();
        },
        playPackagedDemo:async stem=>{
            if(game.session)throw Error('Finish the current sortie before watching a demo.');
            const options=packagedDemoLaunch(assets,stem),sound=activateAudio();music.leaveMenuForFlight({resetCursor:false});
            try{game.session=await createFlight(e,assets,game.career,options);}finally{joystickSettings.force=0;}
            menu.hide();$('flight').hidden=false;$('flight-toolbar').hidden=false;audio.reset();await sound;
            paused=true;pause(false);paint();canvas.focus();
        },
        sounds:async()=>{await sounds.load();soundEditorOptions(menu,sounds);},
        changeEdition:async key=>{if(game.session)throw Error('Finish the current sortie before changing editions.');if(!assets.editions.some(item=>item.key===key))throw Error('This edition is not installed.');await game.career.save();const url=new URL(location.href);url.searchParams.set('edition',key);location.assign(url.href);},
        importMusic:()=>{$('music-file').click();},
        playMusic:async()=>{await activateAudio();const result=await music.enterMenu('moag');if(!result)throw Error(`Select ${music.playlists.moag.join(' or ')} and ${music.profile.bank} first.`);},
        stopMusic:()=>music.stop(),
        title:()=>showTitle(),
        import:()=>{if(!storage)throw Error(storageError);$('import-file').click();}
    });
    builderSave.addEventListener('click',builderSaveHandler({game,menu,assets,canvas,isPaused:()=>paused,pause,isFinishing:()=>finishing,canResume:()=>!document.hidden&&document.hasFocus()}));
    installMoagMenuKeys(menu);
    function showTheaterInfo(){
        menu.preflightKeys=false;menu.keyHandler=null;
        theaterInfo=new TheaterInfo({document,parent:menu.root,game,session:game.session,callbacks:{
            beep:()=>menu.actions.sourceBeep(theaterInfo),
            error:error=>{theaterInfo.message.textContent=error.message;},
            close:()=>menu.actions.resumeMenu()
        }});
        theaterInfo.show();
    }
    const pictures=new ResultPictures({canvas,manifest:assets.resultScenes,baseURL:assets.resultBase,onActive:active=>{if(active){menu.hide();$('flight').hidden=false;$('flight-toolbar').hidden=true;}},onScene:()=>say('Press a key or tap the picture to continue')});
    let titleTick=tick();
    const title=new TitlePresentation({canvas,exports:e,manifest:assets.titleScenes,baseURL:assets.titleBase,
        onActive:active=>{if(active){menu.hide();$('flight').hidden=false;$('flight-toolbar').hidden=true;}},
        onScene:scene=>say(scene.kind==='story'?'Press a key to continue · Esc skips the title':'Title · + / − changes speed · Q toggles sound · Esc continues'),
        nextFrame:()=>new Promise(resolve=>{const frame=now=>{if(document.hidden||!document.hasFocus()){titleTick=tick();requestAnimationFrame(frame);return;}const current=tick();e.cc_audio_clock_advance(Math.min(current-titleTick,65535));titleTick=current;resolve(now);};requestAnimationFrame(frame);}),
        onAudio:async event=>{const action=titleAudioToCore(e,event);if(action==='start-title-music'){try{await music.enterMenu('title');}catch(error){say(`Music is unavailable: ${error.message}`);}}else if(action==='stop-title-music')music.stop();},
        onAudioBatch:()=>audio.flush(),musicPlaying:()=>audio.musicActive
    });
    async function showTitle(){
        if(game.session)throw Error('Finish the current sortie before replaying the title.');
        await activateAudio();music.stop();e.cc_audio_reset();audio.reset();titleTick=tick();
        try{await title.show({music:!!music.defaultTrack('title'),sound:true,seed:1});}
        finally{music.stop();e.cc_audio_sound_off();audio.flush();$('flight').hidden=true;menu.hangar();say('Ready');}
    }
    async function finish(errorChoice){
        if(finishing)return;finishing=true;pause(true,{account:false});
        try{if(game.session.builderDocument){downloadDefinition(await exportBuilderDefinition(game.session,assets.wasm));game.session=null;builderSave.hidden=true;$('flight-toolbar').hidden=true;$('flight').hidden=true;builderOptions(menu);say('Definition exported');return;}const exit=game.session.exit;if(!exit.picturesShown){exit.picturePlan??=resultPicturePlan(e,e.cc_flight_state(),exit.engineError);await pictures.show(exit.picturePlan);exit.picturesShown=true;}const report=await game.finish(errorChoice===undefined?{}:{errorChoice});if(report.needsErrorChoice){menu.errorChoice();return;}
            demos.retain(game.session.demo);game.session=null;$('flight-toolbar').hidden=true;$('flight').hidden=true;if(report.demo){menu.hangar();say(report.engineError?`Demo ended with original engine error ${report.engineError}`:'Demo finished');}else{menu.report(report);say('Sortie complete');}
        }catch(error){
            const recover=error instanceof SaveWriteError&&storage?.persistent?async()=>{
                // Career.finish discarded its failed candidate. Start from the
                // untouched checkpoint and run the same source gate once more.
                const temporary=new MemoryGameStorage(assets.editionId);await temporary.commit(game.career.records,{replace:true});
                storage=game.storage=game.career.storage=temporary;
                await finish(errorChoice);
            }:null;
            menu.failure(error.message,()=>finish(errorChoice),recover);
        }finally{finishing=false;}
    }
    $('pause').addEventListener('click',async()=>{if(!menu.root.hidden||!game.session||game.session.status)return;if(paused){await activateAudio();pause(false);}else{input.key('KeyP',true,tick());input.key('KeyP',false,tick());}canvas.focus();});
    $('tower').addEventListener('click',()=>{if(!menu.root.hidden||!game.session||game.session.status)return;if(paused)pause(false);input.key('F2',true,tick());input.key('F2',false,tick());canvas.focus();});
    $('end').addEventListener('click',()=>{if(!menu.root.hidden||!game.session||game.session.status)return;if(paused)pause(false);input.key('Escape',true,tick());input.key('Escape',false,tick());});
    $('fullscreen').addEventListener('click',async()=>{try{if(document.fullscreenElement)await document.exitFullscreen();else await $('flight-shell').requestFullscreen();}catch{say('Full screen is unavailable in this browser.');}finally{canvas.focus();}});
    $('import-file').addEventListener('change',async()=>{const file=$('import-file').files[0];$('import-file').value='';if(file)await menu.run(async()=>{await game.import(file);await sounds.load();menu.hangar();});});
    $('music-file').addEventListener('change',()=>{const files=[...$('music-file').files];$('music-file').value='';void menu.run(async()=>{const tracks=await music.importFiles(files);menu.message.textContent=`Loaded ${tracks.length} music track${tracks.length===1?'':'s'}.`;});});
    const bound=new Set(['ArrowUp','ArrowDown','ArrowLeft','ArrowRight','Space','Tab','Escape','Home','PageUp','PageDown','End','Insert','Delete','F1','F2','F3','F4','F5','F6','F7','F8','F9','F10','AltLeft','ControlLeft','CapsLock','NumLock']);
    window.addEventListener('keydown',event=>{
        const editable=event.target instanceof HTMLSelectElement||event.target instanceof HTMLInputElement||event.target.tagName==='TEXTAREA';
        if(editable&&(!menu.root.hidden&&menu.keyHandler?.inputKeys?.has(event.code))){menu.key(event);return;}
        if(editable)return;
        const interactive=event.target instanceof HTMLButtonElement||event.target instanceof HTMLAnchorElement;
        if(theaterInfo?.active){if(preflightBeep.capture(theaterInfo,event))return;if(!(interactive&&['Enter','Space'].includes(input.alias(event.code))))theaterInfo.key(event);return;}
        if(!menu.root.hidden){if(preflightBeep.capture(menu,event))return;if(!(interactive&&['Enter','Space'].includes(input.alias(event.code))))menu.key(event);return;}
        if(interactive)return;
        if(bound.has(input.alias(event.code)))event.preventDefault();
        if(paused&&(event.code==='KeyP'||event.code==='Escape')){if(!event.repeat){consumedResumeKeys.add(event.code);void activateAudio();pause(false);}event.preventDefault();return;}
        if(!paused){capturedFlightKeys.add(event.code);input.key(event.code,true,tick());}
    });
    window.addEventListener('keyup',event=>{if(consumedResumeKeys.delete(event.code)){event.preventDefault();return;}if(!capturedFlightKeys.delete(event.code))return;if(bound.has(input.alias(event.code)))event.preventDefault();if(!paused&&menu.root.hidden)input.key(event.code,false,tick());});
    window.addEventListener('blur',()=>{if(game.session)pause(true);if(title.active)void audio.pause();});
    window.addEventListener('focus',()=>{if(title.active){titleTick=tick();void audio.resume();}});
    document.addEventListener('visibilitychange',()=>{if(document.hidden&&game.session)pause(true);if(title.active){titleTick=tick();void (document.hidden?audio.pause():audio.resume());}});
    function animate(now){
        const delta=now-last;last=now;
        if(!paused&&game.session&&!finishing){
            if(delta>250)pause(true);
            else{elapsed+=delta;const frameMs=game.session.frameMilliseconds??FRAME_MS;if(elapsed>=frameMs){
                elapsed%=frameMs;const controls=input.consume(tick());
                if(game.session.ds.getUint8(0xaf6)){
                    controls.joystick=joystick.sample();
                    if(!controls.joystick.connected){pause(true);void recoverFlightJoystick();requestAnimationFrame(animate);return;}
                }
                const code=game.session.step(controls);demos.retain(game.session.demo);speaker.consume(e);paint();audio.flush();$('pause').textContent=game.session.paused?'Resume flight':'Pause';say(game.session.paused?'Paused · release P or Esc to resume; Space cycles help':e.cc_builder_active()?'Builder · M returns to map · Save DEF exports your changes':game.session.map?(e.cc_edition_is_other_worlds()?'Map · B opens the builder · Home centers; Page Up / Down zoom':'Map · Home centers; Page Up / Down zoom'):game.session.radio?'Radio message · keypad + continues':'Flying');
                if(game.session.uiRequest){pause(true,{account:false,preserveSourceKeys:true});if(game.session.uiRequest==='tower')menu.tower();else if(game.session.uiRequest==='calibration')void flightCalibration();else if(game.session.uiRequest==='inflight-score')menu.inflightScore(game.session.modalResults);else if(game.session.uiRequest==='theater-info')showTheaterInfo();else menu.sourceNotice(game.session.uiRequest);}
                else if(code===4)void finish();
                else if(code){pause(true);say(game.session.failure??`The original engine stopped (${code}).`);}
            }}
        }
        requestAnimationFrame(animate);
    }
    menu.welcome();$('flight').hidden=true;$('flight-toolbar').hidden=true;say(storageError?`Saves unavailable: ${storageError}`:'Ready');requestAnimationFrame(animate);
}catch(error){status.textContent=`Could not load Corncob 3D: ${error.message}`;}
