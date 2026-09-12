// Deterministic, synthetic hostile input tests. No personal files or network.
import assert from 'node:assert/strict';
import {RolMusicController} from '../../web/rol-music-controller.mjs';
import {RolMusic} from '../../web/rol-music.mjs';
import {MemoryGameStorage} from '../../web/storage.mjs';
import {validateWorldFile,readWorldDefinition,MAX_WORLD_BYTES} from '../../web/world-records.mjs';
import {parseBoss} from '../../web/boss.mjs';
const tests=[];
const test=(name,fn)=>tests.push([name,fn]);
const bridge={playRol:async()=>{},stopMusic(){}};
test('builder rejects oversized input without reading',async()=>{let reads=0;await assert.rejects(readWorldDefinition({size:MAX_WORLD_BYTES+1,arrayBuffer:async()=>{reads++;return new ArrayBuffer(1);}}));assert.equal(reads,0);});
// Minimal original-format bank and nine-voice song with one note.
export function musicFixture({tempo=120,duration=1,volume=1,pitch=1}={}){
  const bank=new Uint8Array(20);bank.set(new TextEncoder().encode('ADLIB-'),2);
  const b=new DataView(bank.buffer);b.setInt32(12,20,true);b.setInt32(16,20,true);
  const out=new Uint8Array(2048),d=new DataView(out.buffer);let p=0;
  const u16=n=>{d.setUint16(p,n,true);p+=2;},f=n=>{d.setFloat32(p,n,true);p+=4;};
  u16(0);u16(4);p+=40;u16(60);p+=7;out[p++]=1;p+=143;f(tempo);u16(0);
  for(let v=0;v<9;v++){
    p+=15;u16(v===0?1:0);if(v===0){u16(60);u16(duration);}
    p+=15;u16(0);p+=15;u16(v===0?1:0);if(v===0){u16(0);f(volume);}
    p+=15;u16(v===0?1:0);if(v===0){u16(0);f(pitch);}
  }
  return {bank,rol:out.slice(0,p)};
}
test('music rejects oversized selection before reading',async()=>{
  let reads=0;const file={name:'big.rol',size:16*1024*1024+1,arrayBuffer:async()=>{reads++;throw Error('unexpected read');}};
  await assert.rejects(new RolMusicController(bridge).importFiles([file]));assert.equal(reads,0);
});
test('music rejects excessive file count before reading',async()=>{
  let reads=0;const files=Array.from({length:34},(_,i)=>({name:`${i}.rol`,size:1,arrayBuffer:async()=>{reads++;return new ArrayBuffer(1);}}));
  await assert.rejects(new RolMusicController(bridge).importFiles(files));assert.equal(reads,0);
});
test('music rejects tiny tempo before creating unsafe timestamps',()=>{
  const {rol,bank}=musicFixture({tempo:1e-38});assert.throws(()=>new RolMusic(rol,bank).compile(48000));
});
test('music rejects nonfinite volume and pitch',()=>{
  for(const field of ['volume','pitch'])for(const value of [NaN,Infinity,-Infinity]){
    const {rol,bank}=musicFixture({[field]:value});assert.throws(()=>new RolMusic(rol,bank));
  }
});
test('music validates sample rate',()=>{const {rol,bank}=musicFixture();for(const rate of [NaN,Infinity,0,-1])assert.throws(()=>new RolMusic(rol,bank).compile(rate));});
test('valid synthetic music still compiles',()=>{const {rol,bank}=musicFixture();const p=new RolMusic(rol,bank);assert.ok(p.compile(48000).length>0);assert.ok(Number.isSafeInteger(p.durationFrames));});
test('save rejects oversized file without reading',async()=>{let reads=0;await assert.rejects(new MemoryGameStorage().import({size:16*1024*1024+1,text:async()=>{reads++;return '{}';}},()=>{}));assert.equal(reads,0);});
test('save rejects hostile record names and preserves existing data',async()=>{
  const storage=new MemoryGameStorage();await storage.commit(new Map([['BOSS.TXT',new Uint8Array([65])]]));
  for(const name of ['__proto__','../../secret','THT/1.THT/evil','https://evil.invalid']){
    const payload={format:'corncob-source-port-save',version:1,records:[{name,data:'QQ==',sha256:'bad'}]};
    await assert.rejects(storage.import(new Blob([JSON.stringify(payload)]),()=>{}));assert.deepEqual(await storage.read('BOSS.TXT'),new Uint8Array([65]));
  }
});
test('deterministic world and boss malformed-input sweep',()=>{
  let seed=0x434f524e,accepted=0;const next=()=>{seed^=seed<<13;seed^=seed>>>17;seed^=seed<<5;return seed>>>0;};
  for(let i=0;i<2000;i++){
    const data=new Uint8Array(next()%4096);for(let j=0;j<data.length;j++)data[j]=next()&255;
    try{validateWorldFile(data,{plain:!!(i%2)});accepted++;}catch(error){assert.ok(error instanceof Error);}
    parseBoss(data);
  }
  assert.equal(accepted,0);
});
let failed=0;for(const [name,fn]of tests){try{await fn();console.log(`PASS ${name}`);}catch(error){failed++;console.log(`FAIL ${name}: ${error.message}`);}}
console.log(JSON.stringify({tests:tests.length,failed,randomSeed:'0x434f524e',randomCases:2000}));if(failed)process.exitCode=1;
