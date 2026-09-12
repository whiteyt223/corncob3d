// Cross-module browser orchestration under Node. This is an integration gate;
// its behavioral authority is the separately retained original routine traces.
import assert from 'node:assert/strict';import {writeFile} from 'node:fs/promises';
import {loadGameFixture} from './load_game_fixture.mjs';
import {createFlight} from '../web/startup.mjs';
import {pilotSummary} from '../web/game.mjs';
const reports=[];
for(let index=0;index<4;index++){
    const {e,assets,game}=await loadGameFixture();await game.career.create(`Pilot ${index+1}`);await game.open(index);
    const original=game.career.theater().world.slice();game.session=createFlight(e,assets,game.career,{seed:0x123456});
    const session=game.session;
    for(let step=0;step<50;step++){
        const events=session.radio?[{scan:step%2?156:28,fraction:1}]:[];
        const code=session.step({events});assert.equal(code,0,`${index}: step${step} ${session.failure}`);
        if(session.uiRequest)session.leaveMenu();
    }
    const flying=!session.ds.getUint8(0xafc);session.ds.setUint8(0xaee,0); // Source radio-off option for this exit scenario.
    const result=session.step({events:[{scan:129,fraction:1}]});assert.equal(result,4,`${index}: finalization ${session.failure}`);assert.equal(session.exit.raw.length,140);assert.ok(session.exit.world.length>0);
    const report=await game.finish();assert.ok(report.accepted,`${index}: counted ordinary sortie`);assert.equal(pilotSummary(game.career.current()).sorties,1);
    await game.validateRecords(await game.storage.records());await game.career.load();assert.equal(pilotSummary(game.career.current()).sorties,1);
    const accepted=game.career.theater().world.slice();game.session=createFlight(e,assets,game.career,{seed:0x234567});
    const abort=game.session.step({events:[{scan:29,fraction:0},{scan:46,fraction:0}]});assert.equal(abort,4,`abort${index}: ${game.session.failure}`);
    const discarded=await game.finish();assert.equal(discarded.accepted,false);assert.equal(pilotSummary(game.career.current()).sorties,1);assert.deepEqual(game.career.theater().world,accepted);
    reports.push({theater:assets.theaters[index].file_stem,platformSteps:50,simulationFrames:session.frame,autoBoarded:flying,originalBytes:original.length,acceptedWorldBytes:accepted.length,acceptedSorties:1,abortDiscarded:true});
}
const report={scope:'Fresh start → 50 platform steps with original radio waits → original finalization → named career save/reload → abort rollback. Browser orchestration gate, not whole-original mission parity.',scenarios:reports,mismatches:0};await writeFile(new URL('../docs/game-flow-verification.json',import.meta.url),JSON.stringify(report,null,2)+'\n');console.log(report);
