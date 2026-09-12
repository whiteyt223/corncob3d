import assert from 'node:assert/strict';
import {loadGameFixture} from '../load_game_fixture.mjs';
const {game,storage,e}=await loadGameFixture();await game.career.create('Audit fixture');await game.open(0);await game.career.save();
const original=await storage.records(),before=new Uint8Array(e.memory.buffer).slice();
let seed=0x434f524e,accepted=0,rejected=0;
const next=()=>{seed^=seed<<13;seed^=seed>>>17;seed^=seed<<5;return seed>>>0;};
for(let i=0;i<250;i++){
  const records=new Map([...original].map(([k,v])=>[k,v.slice()]));
  const key=i%2?'pilot.scr':[...records.keys()].find(k=>k.startsWith('THT/'));
  const bytes=records.get(key);
  for(let j=0;j<1+i%8;j++)bytes[next()%bytes.length]=next()&255;
  if(i%5===0)records.set(key,bytes.slice(0,next()%bytes.length));
  try{await game.validateRecords(records);accepted++;}catch(error){assert.ok(!(error instanceof WebAssembly.RuntimeError),`parser trapped: ${error.message}`);rejected++;}
  assert.deepEqual(new Uint8Array(e.memory.buffer),before,'validation modified the running game');
}
assert.deepEqual(await storage.records(),original,'validation modified durable records');
console.log(JSON.stringify({seed:'0x434f524e',mutations:250,accepted,rejected,wasmTraps:0,activeMemoryUnchanged:true,durableRecordsUnchanged:true,scope:'validation only; accepted world gameplay is not fuzzed'}));
