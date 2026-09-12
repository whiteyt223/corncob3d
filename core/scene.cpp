#include "viewport.hpp"
#include <stdint.h>

extern "C" {
int32_t cc_ssin(uint32_t);int32_t cc_scos(uint32_t);
void cc_calcmat(const uint16_t*,uint16_t*);void cc_ncalcmat(const uint16_t*,uint16_t*);
void cc_pmvmul(const uint16_t*,const uint16_t*,uint16_t*);
uint16_t* cc_clip_input();uint16_t* cc_clip_output();uint32_t cc_clip_count();uint32_t cc_clip_polygon(uint32_t);
uint16_t* cc_projection_input();uint16_t* cc_projection_output();uint32_t cc_project_point();
uint16_t* cc_raster_vertices();uint32_t cc_draw_polygon(uint32_t,uint32_t);
void cc_reset_spans();uint32_t cc_draw_disc(uint32_t,uint32_t,uint32_t,uint32_t);
uint16_t* cc_line_input();uint16_t* cc_line_output();uint32_t cc_project_line(uint32_t);
void cc_draw_line(int32_t,int32_t,int32_t,int32_t,uint32_t);uint8_t* cc_framebuffer();
}
namespace {
using namespace cc::view;
uint16_t matrix[9],origin[3],vertices[60];
int32_t s(uint16_t v){return v<32768?int32_t(v):int32_t(v)-65536;}
}
extern "C" uint16_t* cc_scene_matrix(){return matrix;}
extern "C" uint16_t* cc_scene_origin(){return origin;}
extern "C" uint16_t* cc_scene_vertices(){return vertices;}
extern "C" void cc_scene_angles(uint32_t yaw,uint32_t pitch,uint32_t roll,uint32_t inverse){
    const uint16_t trig[6]={uint16_t(cc_ssin(yaw)),uint16_t(cc_scos(yaw)),uint16_t(cc_ssin(pitch)),uint16_t(cc_scos(pitch)),uint16_t(cc_ssin(roll)),uint16_t(cc_scos(roll))};
    if(inverse)cc_ncalcmat(trig,matrix);else cc_calcmat(trig,matrix);
}
// Source chain: drpoly's fast transform -> polyhell -> f3dpoly -> drawpoly.
// Main-window solid polygons only; caller supplies original component order.
extern "C" uint32_t cc_render_polygon(uint32_t count,uint32_t color){
    cc_reset_spans();
    if(count<3||count>20||color>255)return 3;
    auto* clipped_input=cc_clip_input();
    for(unsigned i=0;i<count;++i){
        uint16_t rotated[3];cc_pmvmul(matrix,vertices+i*3,rotated);
        for(unsigned j=0;j<3;++j)clipped_input[i*3+j]=uint16_t(uint32_t(rotated[j])+origin[j]);
    }
    auto status=cc_clip_polygon(count);if(status)return status;
    const auto n=cc_clip_count();if(n>20)return 3;
    auto* clipped=cc_clip_output();auto* projection=cc_projection_input();auto* screen=cc_projection_output();auto* raster=cc_raster_vertices();
    projection[3]=window(Zx);projection[4]=window(Zy);projection[5]=window(Cx);projection[6]=window(Cy);
    int minx=32767,miny=32767,maxx=-32767,maxy=-32767;
    for(unsigned i=0;i<n;++i){
        for(unsigned j=0;j<3;++j)projection[j]=clipped[i*3+j];
        status=cc_project_point();if(status)return status;
        const int x=s(screen[0]),y=s(screen[1]);
        if(x<minx)minx=x;
        if(x>maxx)maxx=x;
        if(y<miny)miny=y;
        if(y>maxy)maxy=y;
        raster[2*i]=screen[0];raster[2*i+1]=screen[1];
    }
    if(maxx<=window(Left)||minx>=window(Right)||maxy<=window(Top)||miny>=window(Bottom))return 1;
    return cc_draw_polygon(n,color);
}
// drawrdsc -> f3ddsc, with rdf=0 and the forward main window.
// The source field often called radius is used directly as projected diameter.
extern "C" uint32_t cc_render_disc(uint32_t diameter,uint32_t color){
    uint16_t point[3];cc_pmvmul(matrix,vertices,point);
    for(unsigned j=0;j<3;++j)point[j]=uint16_t(uint32_t(point[j])+origin[j]);
    const int depth=s(point[0]);if(depth<0||s(uint16_t(window(Zx)-depth))>=0)return 1;
    int coordinates[2];
    for(unsigned i=0;i<2;++i){
        int q=s(uint16_t(0u-point[i+1]))*(i?window(Zy):window(Zx))/depth;
        if(q< -32768||q>32767)return 2;
        coordinates[i]=s(uint16_t(q+(i?window(Cy):window(Cx))));
    }
    const int width=s(uint16_t(diameter))*window(Zx)/depth;
    if(width< -32768||width>32767)return 2;
    return cc_draw_disc(uint16_t(coordinates[0]),uint16_t(coordinates[1]),uint16_t(width),color);
}
// drrobj uses its hmul macro (Q15), not drpoly's fast ROL-high transform.
// Supply local tmat and UNROTATED relative xvec; not camera-space origin.
extern "C" uint32_t cc_scene_octant(){
    unsigned octant=0;
    for(unsigned column=0;column<3;++column){
        uint16_t sum=0;
        for(unsigned row=0;row<3;++row){
            int32_t product=s(origin[row])*s(matrix[row*3+column]);
            sum=uint16_t(uint32_t(sum)+(uint32_t(product)>>15));
        }
        octant=(octant<<1)|(sum>>15);
    }
    return octant;
}
extern "C" uint32_t cc_prepare_wire(uint32_t rdf){
    auto* line=cc_line_input();
    for(unsigned i=0;i<2;++i){uint16_t p[3];cc_pmvmul(matrix,vertices+i*3,p);for(unsigned j=0;j<3;++j)line[i*3+j]=uint16_t(uint32_t(p[j])+origin[j]);}
    return cc_project_line(rdf);
}
extern "C" uint32_t cc_render_wire(uint32_t color,uint32_t rdf){
    auto status=cc_prepare_wire(rdf);if(status)return status;
    auto* p=cc_line_output();cc_draw_line(s(p[0]),s(p[1]),s(p[2]),s(p[3]),color);return 0;
}
extern "C" uint32_t cc_render_point(uint32_t color){
    auto* input=cc_projection_input();uint16_t p[3];cc_pmvmul(matrix,vertices,p);
    for(unsigned j=0;j<3;++j)input[j]=uint16_t(uint32_t(p[j])+origin[j]);
    input[3]=window(Zx);input[4]=window(Zy);input[5]=window(Cx);input[6]=window(Cy);
    auto status=cc_project_point();if(status)return status;
    auto* out=cc_projection_output();int x=s(out[0]),y=s(out[1]);if(s(uint16_t(x-window(Left)))<0||s(uint16_t(x-window(Right)))>=0||s(uint16_t(y-window(Top)))<0||s(uint16_t(y-window(Bottom)))>=0)return 1;
    cc_framebuffer()[y*640+x]=uint8_t(color&15);return 0;
}
