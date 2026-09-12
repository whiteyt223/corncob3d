// Original Other Worlds 3demo.dat transport. File ownership and scheduling are
// host boundaries; all record/codec/playback arithmetic is in the source core.
const buffers=new WeakMap();
export function validateDemo(bytes){
    if(!(bytes instanceof Uint8Array))bytes=new Uint8Array(bytes);
    let offset=0,records=0;
    while(offset<bytes.length){
        const flag=bytes[offset];
        if(flag===255)return {records,consumed:offset+1,trailing:bytes.length-offset-1};
        if(!(flag&8))throw Error(`Invalid demo marker at byte ${offset}.`);
        const size=flag&128?8:37;
        if(size>bytes.length-offset)throw Error(`The demo ends inside a record at byte ${offset}.`);
        offset+=size;records++;
    }
    throw Error('The demo has no end marker. Finish recording with U before exporting it.');
}
function storeInput(e,bytes){
    if(!bytes)return 0;
    let slot=buffers.get(e.memory);
    if(!slot||slot.capacity<bytes.length){const pages=Math.ceil(bytes.length/65536);slot={pointer:e.memory.grow(pages)*65536,capacity:pages*65536};buffers.set(e.memory,slot);}
    new Uint8Array(e.memory.buffer,slot.pointer,bytes.length).set(bytes);return slot.pointer;
}
export class DemoStream {
    constructor(e,bytes=null){
        this.e=e;this.input=bytes?.slice()??null;if(this.input)validateDemo(this.input);
        // Construct before FlightSession creates its DataView/runtime views.
        this.pointer=storeInput(e,this.input);this.chunks=[];this.byteLength=0;this.complete=false;this.serial=0;
    }
    reset(){this.chunks=[];this.byteLength=0;this.complete=false;this.serial=0;this.e.cc_demo_frame_reset(this.pointer,this.input?.length??0,Number(!!this.input));}
    get mode(){return this.e.cc_demo_frame_mode();}
    rawTicks(normal){return this.mode===255?this.e.cc_demo_ticks(this.e.cc_flight_state()+0xe8a,normal/8)*8:normal;}
    capture(){
        const io=new Uint32Array(this.e.memory.buffer,this.e.cc_demo_frame_io(),6);
        if(io[4]!==this.serial){
            this.serial=io[4];if(io[1]&1){this.chunks=[];this.byteLength=0;this.complete=false;}
            if(io[0]){const bytes=new Uint8Array(this.e.memory.buffer,this.e.cc_demo_frame_output(),io[0]).slice();this.chunks.push(bytes);this.byteLength+=bytes.length;}
            if(io[1]&2)this.complete=true;
        }
        if(io[5]===1)throw Error('The imported demo ended inside an original read.');
        if(io[5]===2)throw Error('The original demo velocity calculation overflowed.');
    }
    bytes(){const bytes=new Uint8Array(this.byteLength);let at=0;for(const chunk of this.chunks){bytes.set(chunk,at);at+=chunk.length;}return bytes;}
}
export class DemoLibrary {
    constructor(e){this.e=e;this.imported=null;this.name='';this.nextReplay=false;this.recording=null;}
    get enabled(){return !!this.e.cc_edition_is_other_worlds?.();}
    async importFile(file){const bytes=new Uint8Array(await file.arrayBuffer());const info=validateDemo(bytes);this.imported=bytes;this.name=file.name;return info;}
    launchOptions(){const enabled=this.enabled,bytes=enabled&&this.nextReplay?this.imported:null;if(enabled&&this.nextReplay&&!bytes)throw Error('Choose an original demo file first.');this.nextReplay=false;return {demoPlayback:bytes};}
    retain(stream){if(stream?.byteLength)this.recording=stream;}
    exportRecording(){
        if(!this.recording?.byteLength)throw Error('No recording has been made. Release U during an Other Worlds sortie to begin.');
        return this.recording.bytes();
    }
}
