#include "viewport.hpp"
#include "fixed.hpp"
using namespace cc;
extern "C" uint8_t* cc_framebuffer();
namespace {
using namespace cc::view;
uint16_t vectors[6],screen[4];
bool exception,two_d_clip;
int32_t divide(int32_t a,int32_t b){
    if(!b){exception=true;return 0;}
    int32_t q=a/b;if(q< -32768||q>32767)exception=true;return w(q);
}
bool negative(int32_t a,int32_t b){return w(a-b)<0;}
unsigned code(int32_t x,int32_t y){return (negative(x,window(Left))?1:0)|(negative(y,window(Top))?2:0)|(!negative(x,window(Right))&&x!=window(Right)?4:0)|(!negative(y,window(Bottom))&&y!=window(Bottom)?8:0);}
void fix(int32_t* p,unsigned index,unsigned c,unsigned axis){
    const int32_t dx=w(p[2]-p[0]),dy=w(p[3]-p[1]);
    if(!axis){
        if(c&1){p[index+1]=w(divide(w(window(Left)-p[0])*dy,dx)+p[1]);p[index]=window(Left);}
        if(c&4){p[index+1]=w(divide(w(window(Right)-p[0])*dy,dx)+p[1]);p[index]=window(Right);}
    }else{
        if(c&2){p[index]=w(divide(w(window(Top)-p[1])*dx,dy)+p[0]);p[index+1]=window(Top);}
        if(c&8){p[index]=w(divide(w(window(Bottom)-p[1])*dx,dy)+p[0]);p[index+1]=window(Bottom);}
    }
}
void pixel(int x,int y,uint32_t color){if(x>=0&&x<640&&y>=0&&y<350)cc_framebuffer()[y*640+x]=uint8_t(color&15);}
void run(int y,int left,int right,uint32_t color){for(int x=left;x<right;++x)pixel(x,y,color);}
int ab(int n){return n<0?-n:n;}
}
extern "C" uint16_t* cc_line_input(){return vectors;}
extern "C" uint16_t* cc_line_output(){return screen;}
// F3DVEC f3dline -> oclip. Status0 accepted,1 rejected,2 original IDIV trap.
extern "C" uint32_t cc_project_line(uint32_t rdf){
    exception=false;two_d_clip=false;int32_t a[3],b[3],p[4];for(unsigned i=0;i<3;++i){a[i]=w(vectors[i]);b[i]=w(vectors[3+i]);}
    const int32_t da=w(a[0]-window(Zx)),db=w(b[0]-window(Zx));if((da&db)&32768)return 1;
    if((da|db)&32768){
        if(!negative(a[0],b[0]))for(unsigned i=0;i<3;++i){int32_t t=a[i];a[i]=b[i];b[i]=t;}
        if(!negative(window(Zx),b[0]))return 1;
        const int32_t diff=w(window(Zx)-a[0]),den=w(b[0]-a[0]);if(diff<0)return 1;
        a[1]=w(a[1]+divide(w(b[1]-a[1])*diff,den));a[0]=window(Zx)+1;
        a[2]=w(a[2]+divide(w(b[2]-a[2])*diff,den));if(exception)return 2;
    }
    for(unsigned i=0;i<3;++i){vectors[i]=uint16_t(a[i]);vectors[i+3]=uint16_t(b[i]);}
    for(unsigned i=0;i<2;++i){
        const int32_t* v=i?b:a;if(v[0]<=0)return 1;
        if(!rdf&&(!negative(window(Zx),v[0])||!negative(window(Zy),v[0])))return 1;
        p[i*2]=w(divide(w(-v[1])*window(Zx),v[0])+window(Cx));if(exception)return 2;
        p[i*2+1]=w(divide(w(-v[2])*window(Zy),v[0])+window(Cy));if(exception)return 2;
    }
    unsigned c1=code(p[0],p[1]),c2=code(p[2],p[3]);
    if(c1|c2){
        if(c1&c2)return 1;
        two_d_clip=true;fix(p,0,c1,0);if(exception)return 2;fix(p,2,c2,0);if(exception)return 2;
        c1=code(p[0],p[1]);c2=code(p[2],p[3]);
        if(c1|c2){if(c1&c2)return 1;fix(p,0,c1,1);if(exception)return 2;fix(p,2,c2,1);if(exception)return 2;}
    }
    for(unsigned i=0;i<4;++i)screen[i]=uint16_t(p[i]);
    return 0;
}
extern "C" void cc_draw_line(int32_t x1,int32_t y1,int32_t x2,int32_t y2,uint32_t color){
    if(x2<x1){int t=x1;x1=x2;x2=t;t=y1;y1=y2;y2=t;}
    const int dx=x2-x1,dy=ab(y2-y1),sy=y2>=y1?1:-1;
    if(!dy){run(y1,x1,x2,color);return;}
    if(dy<dx&&dx>=30){
        const int q=dx/dy,f=((dx%dy)/2)*65536/dy;int err=0;
        for(int i=0;i<dy;++i){bool big=err>=0;int count=q+big;run(y1,x1,x1+count,color);x1+=count;y1+=sy;err+=f-(big?32768:0);}
    }else if(!dx){for(int y=y1;y!=y2+sy;y+=sy)pixel(x1,y,color);}
    else{
        const bool shallow=dx>=dy;const int major=shallow?dx:dy,minor=shallow?dy:dx;int err=2*minor-major;
        for(int i=0;i<=major;++i){pixel(x1,y1,color);if(shallow)++x1;else y1+=sy;if(err>=0){if(shallow)y1+=sy;else ++x1;err-=2*major;}err+=2*minor;}
    }
}
// HLINE takes original endpoint order to preserve inverted-flight coloring.
extern "C" uint32_t cc_draw_horizon(int32_t x1,int32_t y1,int32_t x2,int32_t y2,uint32_t sky,uint32_t ground){
    if(ab(x2-x1)+ab(y2-y1)<=8)return 1;
    uint32_t left=y2>=y1?ground:sky,right=y2>=y1?sky:ground;
    uint32_t top=x2>=x1?sky:ground,bottom=x2>=x1?ground:sky;
    int ymin=y1<y2?y1:y2,ymax=y1>y2?y1:y2;
    if(ymin>window(Top))for(int y=window(Top);y<=ymin;++y)run(y,window(Left),window(Right)+1,top);
    if(ymax<window(Bottom))for(int y=ymax;y<window(Bottom);++y)run(y,window(Left),window(Right)+1,bottom);
    if(x2<x1){int t=x1;x1=x2;x2=t;t=y1;y1=y2;y2=t;}
    int dx=x2-x1,dy=ab(y2-y1)+1,sy=y2>=y1?1:-1,x=x1,y=y1,err=0;
    if(dy<=dx){
        int q=dx/dy,f=((dx%dy)/2)*65536/dy;
        for(int i=0;i<dy;++i){run(y,window(Left),x,left);run(y,x,window(Right)+1,right);bool big=err>=0;x+=q+big;y+=sy;err+=f-(big?32768:0);}
    }else{
        if(!dx)dx=1;
        int q=dy/dx,f=((dy%dx)/2)*65536/dx;
        for(int i=0;i<dx;++i){bool big=err>=0;for(int k=0;k<q+int(big);++k){run(y,window(Left),x,left);run(y,x,window(Right)+1,right);y+=sy;}++x;err+=f-(big?32768:0);}
    }
    return 0;
}

extern "C" uint32_t cc_line_was_clipped(){return two_d_clip;}
