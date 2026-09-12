// Required static deployment checks; no browser, screenshots or UI inspection.
import assert from 'node:assert/strict';
import {readFile,readdir,stat,writeFile} from 'node:fs/promises';
import {spawnSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';
import path from 'node:path';
const root=fileURLToPath(new URL('../',import.meta.url)),dist=path.join(root,'dist');
const read=name=>readFile(path.join(dist,name)),json=async name=>JSON.parse(await read(name));
const exists=async name=>assert.ok((await stat(path.join(dist,name))).isFile(),name);
const html=await read('index.html');let references=0,modules=0;
const policy=(await read('_headers')).toString();
for(const directive of ["script-src 'self' 'wasm-unsafe-eval'","connect-src 'self' blob:","object-src 'none'","frame-ancestors 'none'",'X-Content-Type-Options: nosniff'])assert.ok(policy.includes(directive),`missing security policy: ${directive}`);
for(const [,name] of html.toString().matchAll(/(?:src|href)="([^"#]+)"/g)){if(/^[a-z]+:/i.test(name))continue;await exists(name);references++;}
const {instance:{exports:e}}=await WebAssembly.instantiate(await read('corncob.wasm'));
const audioExports=new Set(WebAssembly.Module.exports(new WebAssembly.Module(await read('ym3812.wasm'))).filter(x=>x.kind==='function').map(x=>x.name));
for(const name of await readdir(dist))if(name.endsWith('.mjs')){
    const code=(await read(name)).toString(),check=spawnSync(process.execPath,['--check',path.join(dist,name)],{encoding:'utf8'});
    assert.equal(check.status,0,`${name}: ${check.stderr}`);modules++;
    for(const [,relative] of code.matchAll(/(?:from\s*|import\s*\()\s*['"](\.\.?\/[^'"]+)['"]/g)){await exists(path.join(path.dirname(name),relative));references++;}
    for(const [,fn] of code.matchAll(/\be\.(cc_\w+)\s*\(/g))assert.ok(fn.startsWith('cc_ym3812_')?audioExports.has(fn):typeof e[fn]==='function',`${name}: missing core export ${fn}`);
}
for(const name of ['world.json','initial-ds.bin','cockpit.indices','initial-pilot.scr','theater-definitions.bin','theaters.json','briefings.json','ground-tables.bin','mode-palette.bin','pilot-menu-labels.json','ym3812.wasm','credits.txt','manual.txt','adplug-license.txt'])await exists(name);
const theaters=(await json('theaters.json')).records;
for(const {file_stem} of theaters){await exists(`missions/${file_stem}.cct`);await exists(`missions/${file_stem}.twr`);}
await exists('missions/deftower.twr');
const title=await json('presentation/title-presentation-original.json');
assert.equal((await read(`presentation/${title.data.file}`)).length,title.data.bytes);
for(const card of title.cards)assert.equal((await read(`presentation/${card.frame}`)).length,64768);
const all=[];async function walk(directory){for(const item of await readdir(directory,{withFileTypes:true})){const file=path.join(directory,item.name);if(item.isDirectory())await walk(file);else all.push(file);}}await walk(dist);
assert.ok(!all.some(name=>/\.(rol|bnk|exe|com|asm|zip|tar|gz)$/i.test(name)),'Research executables/archives or user-imported music entered the static package');
const report={passed:true,javascriptModules:modules,localReferences:references,theaters:theaters.length,files:all.length,coreExports:Object.keys(e).length,browserTest:false};
await writeFile(path.join(root,'build/bundle-check.json'),JSON.stringify(report,null,2)+'\n');console.log(JSON.stringify(report));
