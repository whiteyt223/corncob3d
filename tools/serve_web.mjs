// Serve the existing static build for browser QA; no production runtime needed.
import {createServer} from 'node:http';
import {readFile} from 'node:fs/promises';
import {resolve,extname,sep} from 'node:path';
import {fileURLToPath} from 'node:url';
const args=process.argv.slice(2),option=(name,fallback)=>args.includes(name)?args[args.indexOf(name)+1]:fallback;
const root=fileURLToPath(new URL('../dist/',import.meta.url));
const types={'.html':'text/html','.mjs':'text/javascript','.css':'text/css','.wasm':'application/wasm','.json':'application/json','.woff':'font/woff','.txt':'text/plain'};
// Mirror the bundle's single /* policy during QA. Cloudflare parses _headers.
const securityHeaders=Object.fromEntries((await readFile(resolve(root,'_headers'),'utf8')).split('\n').filter(line=>/^\s+[^:]+:/.test(line)).map(line=>{const at=line.indexOf(':');return [line.slice(0,at).trim(),line.slice(at+1).trim()];}));
createServer(async(req,res)=>{
  try{
    const pathname=decodeURIComponent(new URL(req.url,'http://localhost').pathname);
    if(pathname==='/_headers'){res.writeHead(404).end();return;}
    const file=resolve(root,'.'+(pathname==='/'?'/index.html':pathname));
    if(!file.startsWith(resolve(root)+sep)){res.writeHead(403).end();return;}
    const bytes=await readFile(file);
    res.writeHead(200,{...securityHeaders,'Content-Type':types[extname(file)]??'application/octet-stream','Cache-Control':'no-store'});
    res.end(req.method==='HEAD'?undefined:bytes);
  }catch{res.writeHead(404).end('Not found');}
}).listen(Number(option('--port','4173')),option('--host','127.0.0.1'));
