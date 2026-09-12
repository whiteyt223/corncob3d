#include "viewport.hpp"
#include "state.hpp"
using namespace cc;
using namespace cc::view;
extern "C" {
uint8_t* cc_framebuffer();uint8_t* cc_video_memory();void cc_video_load(uint32_t);
uint32_t cc_line_was_clipped();uint16_t* cc_line_input();uint16_t* cc_line_output();uint32_t cc_project_line(uint32_t);
void cc_draw_line(int32_t,int32_t,int32_t,int32_t,uint32_t);
uint32_t cc_draw_horizon(int32_t,int32_t,int32_t,int32_t,uint32_t,uint32_t);
uint32_t cc_draw_disc(uint32_t,uint32_t,uint32_t,uint32_t);uint32_t cc_disc_carry(uint32_t,uint32_t,uint32_t);
uint16_t* cc_clip_input();uint16_t* cc_clip_output();uint32_t cc_clip_count();uint32_t cc_clip_polygon(uint32_t);
uint16_t* cc_raster_vertices();uint32_t cc_draw_polygon(uint32_t,uint32_t);void cc_reset_spans();
uint16_t* cc_scene_matrix();uint16_t* cc_scene_origin();uint16_t* cc_scene_vertices();
void cc_pmvmul(const uint16_t*,const uint16_t*,uint16_t*);
}
namespace {
uint16_t camera[126];uint32_t result[6];uint16_t front_words[12]={8,4,631,199,623,195,300,200,319,101,321,15920};bool rear=false,horizon=false,filled=true,diving=false,invalid_horizon_window=false;uint32_t sky=11,ground=2;
void begin(){invalid_horizon_window=false;for(auto& n:result)n=0;result[1]=~0u;set(front_words);}
void rear_vector(uint16_t* p){if(horizon)p[2]=uint16_t(0u-p[2]);else{p[0]=uint16_t(0u-p[0]);p[1]=uint16_t(0u-p[1]);}}
void objfail(){++result[2];state::wb(0x1fc7,state::b(0x1fc7)+1);}
uint32_t point(const uint16_t* p,uint32_t color,uint16_t* diameter=nullptr){
    const int depth=w(p[0]);if(depth<0||w(window(Zx)-depth)>=0)return 1;
    int out[2];
    for(unsigned i=0;i<2;++i){
        if(w(window(Zx+i)-depth)>=0)return 1;
        if(!depth)return 2;
        int q=w(-int(p[1+i]))*window(Zx+i)/depth;if(q< -32768||q>32767)return 2;
        out[i]=w(q+window(Cx+i));
        if(!i&&diameter){q=w(*diameter)*window(Zx)/depth;if(q< -32768||q>32767)return 2;*diameter=uint16_t(q);}
    }
    if(diameter){uint32_t status=cc_draw_disc(uint16_t(out[0]),uint16_t(out[1]),*diameter,color);return status==2?2:cc_disc_carry(out[0],out[1],*diameter);}
    if(w(out[0]-window(Left))<0||w(out[0]-window(Right))>=0||w(out[1]-window(Top))<0||w(out[1]-window(Bottom))>=0)return 1;
    const unsigned address=uint16_t(out[1]*80+(uint16_t(out[0])>>3));
    if(address*8+(unsigned(out[0])&7)<640*350)cc_framebuffer()[address*8+(unsigned(out[0])&7)]=uint8_t(color&15);
    return 0;
}
uint32_t polygon(unsigned& count,uint32_t color,unsigned rdf){
    cc_reset_spans();if(count<3||count>20||color>255)return 3;
    auto* input=cc_clip_input();for(unsigned i=0;i<126;++i)input[i]=camera[i];
    uint32_t status=cc_clip_polygon(count);
    for(unsigned i=0;i<126;++i)camera[i]=input[i];
    if(status)return status;
    count=cc_clip_count();if(count>20)return 3;
    auto* clipped=cc_clip_output();auto* screen=cc_raster_vertices();
    int minx=32767,miny=32767,maxx=-32767,maxy=-32767;
    for(unsigned i=0;i<count;++i){
        int depth=w(clipped[i*3]);if(depth<0)return 1;
        for(unsigned j=0;j<2;++j){
            if(!rdf&&w(window(Zx+j)-depth)>=0)return 1;
            if(!depth)return 2;
            int q=w(-int(clipped[i*3+1+j]))*window(Zx+j)/depth;if(q< -32768||q>32767)return 2;
            screen[i*2+j]=uint16_t(q+window(Cx+j));
        }
        int x=w(screen[i*2]),y=w(screen[i*2+1]);if(x<minx)minx=x;if(x>maxx)maxx=x;if(y<miny)miny=y;if(y>maxy)maxy=y;
    }
    if(maxx<=window(Left)||minx>=window(Right)||maxy<=window(Top)||miny>=window(Bottom))return 1;
    return cc_draw_polygon(count,color);
}
uint32_t line(uint32_t color,unsigned rdf){
    auto* input=cc_line_input();for(unsigned i=0;i<6;++i)input[i]=camera[i];
    uint32_t status=cc_project_line(rdf);for(unsigned i=0;i<6;++i)camera[i]=input[i];
    if(status)return status;
    auto* s=cc_line_output();
    if(horizon&&filled&&!cc_line_was_clipped()&&absw(w(s[2]-s[0]))+absw(w(s[3]-s[1]))>8){invalid_horizon_window=true;return 0;}
    if(horizon&&filled)return cc_draw_horizon(w(s[0]),w(s[1]),w(s[2]),w(s[3]),sky,ground);
    cc_draw_line(w(s[0]),w(s[1]),w(s[2]),w(s[3]),color);return 0;
}
void fallback(bool tail){
    uint8_t color=uint8_t((tail?(diving?sky:ground):(diving?ground:sky))&15);
    const unsigned start=unsigned(tail?20341:window(MinByte))*8;
    const unsigned width=tail?304:((unsigned(window(Width))+1)>>4)*16;
    const unsigned height=tail?90:unsigned(window(Height));
    for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){unsigned at=start+y*640+x;if(at<640*350)cc_framebuffer()[at]=color;}
}
void transform(unsigned count){
    for(unsigned i=0;i<count;++i){uint16_t p[3];cc_pmvmul(cc_scene_matrix(),cc_scene_vertices()+i*3,p);for(unsigned j=0;j<3;++j)camera[i*3+j]=uint16_t(p[j]+cc_scene_origin()[j]);}
}
// All-one mode3 writes, so no latch copying is involved. Page address wraps
// at 65536 EGA bytes. thckbar uses LOOP: zero CX means 65536 writes.
void video_byte(unsigned address,unsigned page,unsigned color){
    auto* v=cc_video_memory()+uint16_t(address+(page?0:0x7e00))*8;
    for(unsigned i=0;i<8;++i)v[i]=uint8_t(color&15);
}
void thick(unsigned address,unsigned count,unsigned page,unsigned color){
    const unsigned n=uint16_t(count)?uint16_t(count):65536;
    for(unsigned i=0;i<n;++i)video_byte(address+i*80,page,color);
}
void box(unsigned address,unsigned width,unsigned rows,unsigned page,unsigned color){
    for(unsigned y=0;y<rows;++y)for(unsigned x=0;x<width;++x)video_byte(address+y*80+x,page,color);
}
}
extern "C" const uint16_t* cc_viewport_words(uint32_t id){return id<3?presets[id]:nullptr;}
extern "C" uint16_t* cc_viewport_active(){return active;}
extern "C" uint32_t cc_viewport_select(uint32_t id){if(id>=3)return 1;set(presets[id]);return 0;}
extern "C" void cc_viewport_set(const uint16_t* words){set(words);}
extern "C" void cc_views_configure(uint32_t full,uint32_t tail){for(unsigned i=0;i<12;++i)front_words[i]=presets[full?1:0][i];rear=tail!=0;set(front_words);}
extern "C" void cc_views_custom_front(const uint16_t* words){for(unsigned i=0;i<12;++i)front_words[i]=words[i];set(front_words);}
extern "C" void cc_views_flags(uint32_t h,uint32_t d,uint32_t up,uint32_t down){horizon=h!=0;diving=(d&128)!=0;sky=up;ground=down;}
extern "C" uint16_t* cc_views_input(){return camera;}
extern "C" uint32_t* cc_views_result(){return result;}
extern "C" void cc_views_fill(uint32_t enabled){filled=enabled!=0;}
extern "C" uint32_t cc_view_point_front(uint32_t color){return point(camera,color);}
extern "C" uint32_t cc_view_point(uint32_t color){
    begin();result[0]=point(camera,color);if(result[0]==2)return 2;if(result[0])objfail();
    if(rear){rear_vector(camera);set(presets[2]);result[1]=point(camera,color);if(result[1]==1)objfail();}
    set(front_words);return result[1]==2?2:0;
}
extern "C" uint32_t cc_view_disc(uint32_t diameter,uint32_t color){
    begin();uint16_t width=uint16_t(diameter);result[0]=point(camera,color,&width);result[4]=width;if(result[0]==2)return 2;if(result[0])objfail();
    if(rear){rear_vector(camera);set(presets[2]);result[1]=point(camera,color,&width);if(result[1]==1)objfail();}
    result[4]=width;set(front_words);return result[1]==2?2:0;
}
extern "C" uint32_t cc_view_line(uint32_t color,uint32_t rdf){
    begin();uint16_t original[6];for(unsigned i=0;i<6;++i)original[i]=camera[i];
    result[0]=line(color,rdf);if(result[0]==2)return 2;
    result[3]=result[0]?2:0;if(result[0]&&horizon)fallback(false);
    if(rear){result[3]>>=1;for(unsigned i=0;i<6;++i)camera[i]=original[i];rear_vector(camera);rear_vector(camera+3);set(presets[2]);result[1]=line(color,rdf);if(result[1]==1){++result[3];if(horizon)fallback(true);}}
    state::wb(0x1fc8,result[3]);set(front_words);return result[1]==2?2:invalid_horizon_window?3:0;
}
extern "C" uint32_t cc_view_polygon(uint32_t count,uint32_t color,uint32_t rdf){
    begin();unsigned n=count;result[0]=polygon(n,color,rdf);result[5]=n;if(result[0]>=2)return result[0];
    if(rear){for(unsigned i=0;i<n;++i)rear_vector(camera+i*3);set(presets[2]);result[1]=polygon(n,color,rdf);}
    result[5]=n;set(front_words);return result[1]==2?2:0;
}
extern "C" uint32_t cc_render_views_point(uint32_t color){transform(1);return cc_view_point(color);}
extern "C" uint32_t cc_render_views_disc(uint32_t diameter,uint32_t color){transform(1);return cc_view_disc(diameter,color);}
extern "C" uint32_t cc_render_views_wire(uint32_t color,uint32_t rdf){transform(2);auto status=cc_view_line(color,rdf);if(!status&&result[3]>1)objfail();return status;}
extern "C" uint32_t cc_render_views_polygon(uint32_t count,uint32_t color,uint32_t rdf){if(count>20)return 3;transform(count);return cc_view_polygon(count,color,rdf);}
extern "C" uint32_t cc_resolve_draw_color(uint32_t word){return word&0xff00?state::b(uint16_t(word)):word&255;}
extern "C" uint32_t cc_epage_page(uint32_t page,uint32_t color,uint32_t tail,uint32_t jump,uint32_t value){
    page&=1;unsigned left=uint16_t(window(MinByte)-1),height=unsigned(window(Height));
    thick(left,height,page,color);thick(left+79,height,page,color);
    if(jump){unsigned lines=height*uint16_t(value)/1024;if(lines>65535){cc_video_load(page);return 2;}thick(left+79+uint16_t(height-lines)*80,lines,page,5);}
    box(0,640,1,page,color);box(unsigned(window(MaxByte)),640,1,page,color);
    if(tail){box(0x4de4,40,5,page,color);box(0x6b94,40,5,page,color);thick(0x4f74,91,page,color);thick(0x4f9b,91,page,color);}
    cc_video_load(page);return 0;
}
extern "C" uint32_t cc_epage_state_page(uint32_t page){
    uint16_t saved[12];for(unsigned i=0;i<12;++i)saved[i]=active[i];
    unsigned ptr=state::u(0x1c3e);for(unsigned i=0;i<12;++i)active[i]=state::u(ptr+i*2);
    auto status=cc_epage_page(page,state::b(0x2158),state::u(0x1c40)!=0,state::b(0x0f7c),state::u(0x0f7d));
    set(saved);return status;
}
