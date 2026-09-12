// Original Corncob retail3.42 dynamic-world reference helpers.
// Derived from 3.ASM grndpt/drandr/frame loop, F3DVEC modhr/xvect,
// and executable rand_w atCS:e860. See dynamic-world-fixtures.json.
export const signedWord=v=>(v<<16)>>16;
const signedByte=v=>(v<<24)>>24;
export function originalRandom(initialSeed=147867123){
    let seed=initialSeed>>>0;
    return {
        get seed(){return seed;},
        set seed(value){seed=value>>>0;},
        nextWord(){seed=Math.imul(seed,663608941)>>>0;return seed>>>16;}
    };
}
export function regenerateGroundPoint(point,observer,random){
    const height=observer[2]|0;
    if(height<0)return false;
    const extent=(height<<6)>>>0;
    const half=(extent|0)>>1;
    // extent*r is <2^48, so JS Number preserves this integer product exactly.
    const coordinate=center=>(center+Math.floor(extent*random.nextWord()/65536)-half)|0;
    point.position_cm=[coordinate(observer[0]),coordinate(observer[1]),0];
    point.fixed_flag=0;
    return true;
}
// drawPoint must report original draw success. Called once per ORIGINAL game
// frame, not once per display refresh. Mutate a runtime copy of captured points.
export function drawAndRefreshGround(points,observer,random,drawPoint){
    for(const point of points){
        if(!drawPoint(point))regenerateGroundPoint(point,observer,random);
        if(random.nextWord()<=91)regenerateGroundPoint(point,observer,random);
    }
}
export function horizonDivingByte(viewPitch){
    return (-(signedByte((viewPitch&65535)>>>8)>>1))|0;
}
export function horizonFallback(viewPitch,rear=false,sky=11,ground=2){
    return (horizonDivingByte(viewPitch)<0)!==rear?ground:sky;
}
export function rearVertex(front,horizon=false){
    const [x,y,z]=front.map(signedWord);
    return horizon?[x,y,signedWord(-z)]:[signedWord(-x),signedWord(-y),z];
}
// Original positive tick-count domain; ofrmticks must be nonzero.
export function advanceLook(current,desired,ticksf,ofrmticks){
    if(!ofrmticks)throw new RangeError('Original uwtadj would divide by zero');
    const denominator=Math.max(2,Math.trunc(2*signedWord(ticksf)/signedWord(ofrmticks)));
    return current.map((value,i)=>i<2?signedWord(value+Math.trunc(signedWord(desired[i]-value)/denominator)):signedWord(value));
}
// math.anglesMatrix(angles,inverse), math.multiply(a,b), math.matrixAngles(m)
// must be the original integer/Q15 functions, not floating-point substitutes.
export function observerMatrices(planeAngles,lookAngles,lookEnabled,math){
    const planeForward=math.anglesMatrix(planeAngles,false);
    const planeInverse=math.anglesMatrix(planeAngles,true);
    const observerInverse=lookEnabled?math.multiply(planeInverse,math.anglesMatrix(lookAngles,true)):planeInverse;
    const observerAngles=lookEnabled?math.matrixAngles(observerInverse):planeAngles.slice();
    const observerForward=lookEnabled?math.anglesMatrix(observerAngles,false):planeForward;
    return {planeForward,planeInverse,observerForward,observerInverse,observerAngles,viewPitch:observerAngles[1]};
}
