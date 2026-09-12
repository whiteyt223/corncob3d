// Strict browser file boundary. The original DOS reader deliberately tolerates
// short successful reads; imported files must contain their complete records.
export const MAX_WORLD_BYTES=64*(16+150*23)+12;
export async function readWorldDefinition(file){
    if(!Number.isSafeInteger(file?.size)||file.size<64*16+12||file.size>MAX_WORLD_BYTES)throw Error('Invalid definition size (maximum 221,836 bytes).');
    const bytes=new Uint8Array(await file.arrayBuffer());
    validateWorldFile(bytes,{plain:true});return bytes;
}
export function validateWorldFile(file,{plain=false,offset=0}={}){
    const data=new DataView(file.buffer,file.byteOffset,file.byteLength);let at=offset;
    if(!Number.isInteger(at)||at<0||at>file.length)throw Error('Invalid world file offset.');
    for(let y=0;y<8;y++)for(let x=0;x<8;x++){
        if(file.length-at<16)throw Error('A theater contains an incomplete tile header.');
        if(data.getUint16(at+2,true)!==0x65||data.getUint16(at+4,true)!==x||data.getUint16(at+6,true)!==y)throw Error('A theater tile header is invalid.');
        const count=data.getUint16(at+8,true);at+=16;
        if(count>150||file.length-at<count*23)throw Error('A theater contains incomplete object data.');
        for(let i=0;i<count;i++,at+=23){
            const type=data.getUint16(at+21,true)^(plain?0:data.getUint16(at+2,true)^data.getUint16(at+4,true));
            if(type>60)throw Error('A theater contains an unsupported object type.');
        }
    }
    if(file.length-at!==12)throw Error('A theater contains incomplete or extra world data.');
    return true;
}
