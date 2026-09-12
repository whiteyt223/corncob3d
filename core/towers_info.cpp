#include "towers_info.hpp"
#include "edition.hpp"
#include "airfield_summary.hpp"
extern "C" uint32_t cc_airfield_analyze_file_tu(const uint8_t*,uint32_t,uint32_t);
namespace {
#include "towers_tables.inc"
struct Object {int32_t x,y,z;uint16_t type,flags;uint8_t group;};
Object objects[9600],selected[3855];unsigned starts[64],counts[64];
uint8_t output[1048576];unsigned used;bool failed;bool tu_mode;
unsigned w(const uint8_t*p){return p[0]|unsigned(p[1])<<8;}
int32_t l(const uint8_t*p){return int32_t(w(p)|(w(p+2)<<16));}
void sw(uint8_t*p,unsigned n){p[0]=uint8_t(n);p[1]=uint8_t(n>>8);}
void sl(uint8_t*p,int32_t n){sw(p,uint32_t(n));sw(p+2,uint32_t(n)>>16);}
Object unpack(const uint8_t*p){return {l(p),l(p+4),l(p+8),uint16_t(w(p+12)),uint16_t(w(p+14)),p[16]};}
void pack(uint8_t*p,const Object&o){sl(p,o.x);sl(p+4,o.y);sl(p+8,o.z);sw(p+12,o.type);sw(p+14,o.flags);p[16]=o.group;}
unsigned length(const char*s){unsigned n=0;while(s[n])++n;return n;}
void reset(){used=0;failed=false;output[0]=0;}
void ch(char c){if(used+1>=sizeof output){failed=true;return;}output[used++]=uint8_t(c);output[used]=0;}
void text(const char*s){while(*s)ch(*s++);}
void field(const char*s,unsigned width,unsigned maximum,bool left){unsigned n=length(s);if(n>maximum)n=maximum;if(!left)for(unsigned i=n;i<width;i++)ch(' ');for(unsigned i=0;i<n;i++)ch(s[i]);if(left)for(unsigned i=n;i<width;i++)ch(' ');}
void integer(char*out,int32_t v){char tmp[11];unsigned n=0,k=0;uint32_t q=v<0?0u-uint32_t(v):uint32_t(v);if(v<0)out[k++]='-';do{tmp[n++]=char('0'+q%10);q/=10;}while(q);while(n)out[k++]=tmp[--n];out[k]=0;}
void append(char*dest,const char*s){unsigned n=length(dest);while(*s)dest[n++]=*s++;dest[n]=0;}
int32_t abs32(int32_t v){return v<0?int32_t(0u-uint32_t(v)):v;}
int32_t sub(int32_t a,int32_t b){return int32_t(uint32_t(a)-uint32_t(b));}
int32_t mul(int32_t a,int32_t b){return int32_t(uint32_t(a)*uint32_t(b));}
bool nearby(const Object&a,const Object&b){return abs32(sub(a.x,b.x))<400000&&abs32(sub(a.y,b.y))<400000;}
int compare(const Object&a,const Object&b){
    int d=int(a.group&31)-int(b.group&31);if(d)return d>0?1:-1;
    d=int(types[a.type].flags&2)-int(types[b.type].flags&2);if(d)return d>0?1:-1;
    d=int(a.group&64)-int(b.group&64);if(d)return d>0?1:-1;
    if((a.group&64)&&(b.group&64)){d=int(b.flags&512)-int(a.flags&512);if(d)return d>0?1:-1;}
    d=int(a.type)-int(b.type);if(d)return d>0?1:-1;
    d=int(a.flags&0x7000)-int(b.flags&0x7000);return d?(d>0?1:-1):0;
}
void swap(int a,int b){Object t=selected[a];selected[a]=selected[b];selected[b]=t;}
// Original Turbo C qsort4CF2, including the order of equal-key records.
void sort(int first,int count){
    while(count>2){
        int last=first+count-1,middle=first+count/2;
        if(compare(selected[middle],selected[last])>0)swap(middle,last);
        if(compare(selected[middle],selected[first])>0)swap(middle,first);
        else if(compare(selected[first],selected[last])>0)swap(first,last);
        if(count==3){swap(first,middle);return;}
        int equal=first+1,i=equal;
        for(;;){
            int d=compare(selected[i],selected[first]);
            if(d<=0){
                if(d==0){swap(i,equal);++equal;}
                if(i>=last)break;
                ++i;continue;
            }
            if(i<last){
                for(;;){
                    d=compare(selected[first],selected[last]);
                    if(d<0){--last;if(i<last)continue;}
                    else{swap(i,last);if(d!=0){++i;--last;}}
                    break;
                }
            }
            if(i>=last)break;
        }
        if(compare(selected[i],selected[first])<=0)++i;
        int a=first,b=i-1;
        while(a<equal&&b>=equal){swap(a,b);++a;--b;}
        int left=i-equal,right=first+count-i;
        if(right<left){sort(i,right);count=left;}
        else{sort(first,left);first=i;count=right;}
    }
    if(count==2&&compare(selected[first],selected[first+1])>0)swap(first,first+1);
}
void group(unsigned count){
    unsigned next=1;
    for(unsigned i=0;i<count;i++)if(selected[i].type==6){
        unsigned g=next++;selected[i].group|=uint8_t(g);
        for(unsigned j=0;j<count;j++)if(!(types[selected[j].type].flags&2)&&nearby(selected[i],selected[j]))selected[j].group|=uint8_t(g);
        break;
    }
    for(unsigned i=0;i<count;i++){
        Object&o=selected[i];unsigned quality=o.flags&0x7000;
        if(o.type!=0&&o.type!=43&&o.type!=53&&!(o.type==58&&(quality==0x4000||quality==0x7000)))continue;
        o.group|=64;unsigned g=o.group&31;
        if(!g){g=next++;o.group|=uint8_t(g);}
        for(unsigned j=0;j<count;j++)if(!(selected[j].group&31)&&nearby(o,selected[j]))selected[j].group|=uint8_t(g);
    }
    for(unsigned i=0;i<count;i++)if(!(selected[i].group&31))selected[i].group|=uint8_t(next);
    sort(0,int(count));
}
void direction(char*out,int32_t a,int32_t b){
    const char*pair;
    if(a>0){if(b>0)pair="NE";else if(b<0)pair="SE";else{append(out,"E");return;}}
    else if(a<0){if(b>0)pair="NW";else if(b<0)pair="SW";else{append(out,"W");return;}}
    else{append(out,b>0?"N":b<0?"S":"na");return;}
    a=abs32(a);b=abs32(b);
    bool bigger=b>a;int32_t ratio=bigger?mul(b,100)/a:mul(a,100)/b;
    char v[4]={0,0,0,0};
    if(ratio>503)v[0]=pair[bigger?0:1];
    else if(ratio>150){v[0]=pair[bigger?0:1];v[1]=pair[0];v[2]=pair[1];}
    else{v[0]=pair[0];v[1]=pair[1];}
    append(out,v);
}
void range(char*out,int32_t x,int32_t y){
    int32_t square=int32_t(uint32_t(mul(x,x))+uint32_t(mul(y,y)));int16_t value=int16_t(uint32_t(abs32(x))+uint32_t(abs32(y)));
    if(!square)value=0;
    else for(unsigned n=0;n<100;n++){
        int16_t old=value;if(!old){failed=true;value=0;break;}
        int16_t sum=int16_t(uint16_t(old)+uint16_t(square/old));value=int16_t(int32_t(sum)/2);
        if(value==old)break;
    }
    integer(out,value);unsigned n=length(out);out[n+1]=0;out[n]=out[n-1];out[n-1]='.';
}
void format(const Object&o,int32_t ax,int32_t ay){
    if(o.type>=61||!(types[o.type].flags&1))return;
    char name[160]={},num[20],dir[4]={},distance[16]={};bool quality=false;unsigned q=(o.flags>>12&7)+1;
    if(types[o.type].flags&2)append(name,"Enemy ");
    append(name,types[o.type].name);
    switch(o.type){
    case 0:if(q==5)append(name," (Indestructible)");else if(o.flags&0x800)append(name," (Gravity)");break;
    case 2:append(name," (");append(name,q==1?"Corsair":q==2?"Dumpy":q==3?"Stealth":"Unknown type");append(name,")");if(o.flags&0x100)append(name," (Wrecked)");break;
    case 7:if(!tu_mode&&(o.flags&0x800))append(name," (Spy)");if(q==2)append(name," (Dead)");break;
    case 12:quality=true;if((o.flags&255)==0x71)append(name," (Dead)");else if(o.flags&0x800)append(name," (bumpers)");break;
    case 32:if(q!=1)return;break;
    case 49:append(name," (");append(name,popups[q-1]);append(name,")");break;
    case 11:case 13:case 23:case 24:case 50:case 51:case 53:case 58:quality=true;break;
    }
    if(quality){field(name,27,27,true);text(" Type \\");ch(char('0'+q));text("   ");}
    else{field(name,36,36,true);ch(' ');}
    int32_t dx=sub(o.x,ax),dy=sub(o.y,ay);
    direction(dir,int32_t(0u-uint32_t(dy)),dx);range(distance,dx/16090,dy/16090);
    field(distance,4,4,false);ch(' ');field(dir,3,65535,false);text("    ");integer(num,mul(o.z,10)/305);field(num,5,65535,false);
    text("             (");integer(num,o.group&31);text(num);text(")\r\n");
    const char*mission=nullptr;
    if(o.type==0||o.type==43)mission=missions[q-1];
    else if(o.type==58&&(q==5||q==8))mission="Fly through this portal.";
    else if(o.type==53)mission="Blast this saucer out of the sky.";
    else if(!tu_mode&&o.type==7&&(o.flags&0x800))mission="Get secret document from spy.";
    if(mission&&!(o.flags&512)){text("\\(         Mission Objective:  ");text(mission);text("\\)\r\n");}
}
}
extern "C" uint8_t* cc_towers_text(){return output;}
extern "C" uint32_t cc_towers_text_size(){return used;}
extern "C" const char* cc_towers_field_name(uint32_t i,uint32_t part){return i<9?(part?fieldLast[i]:fieldFirst[i]):"";}
extern "C" uint32_t cc_towers_start(uint8_t*f,uint32_t size,uint32_t i){if(size<12||i>=9)return 1;sl(f+size-12,int32_t(uint32_t(-5043272)+4194304u*(i%3)));sl(f+size-8,int32_t(uint32_t(-4194304)+4194304u*(i/3)));return 0;}
extern "C" int32_t cc_towers_initial(const uint8_t*f,uint32_t size,uint32_t single){
    if(single)return 4;
    if(size<12)return -1;
    int x=int16_t(int32_t(uint32_t(l(f+size-12))+0x8cf448u)/4194304-1);
    int y=int16_t(int32_t(uint32_t(l(f+size-8))+0x800000u)/4194304-1);
    return int16_t(x+3*y);
}
extern "C" uint32_t cc_towers_airfield_key(uint32_t i,uint32_t single,uint32_t key){
    if(i>=9)return i|0x20000;
    if(single)return i;
    unsigned x=i%3,y=i/3;
    switch(key){case 16:case 107:x=(x+1)%3;break;case 14:case 106:x=(x+2)%3;break;case 6:case 108:y=(y+2)%3;break;case 2:case 104:y=(y+1)%3;break;default:return i|0x20000;}
    return x+3*y;
}
extern "C" uint32_t cc_towers_view_key(uint32_t t,uint32_t n,uint32_t key){
    int top=int16_t(t),maximum=int16_t(n)>15?int16_t(n)-15:0;unsigned beep=0,close=0,ret=0;
    switch(uint16_t(key)){
    case 8:case 10:case 27:case 66:case 73:case 78:case 98:case 105:case 110:case 113:
        close=1;ret=key==10?1:((key|32)==110||(key|32)==105||(key|32)==98)?key:0;break;
    case 6:case 14:case 50:case 54:case 106:if(++top>maximum){beep=1;top=maximum;}break;
    case 2:case 16:case 52:case 56:case 107:if(--top<0){beep=1;top=0;}break;
    case 25:case 32:case 100:if(top==maximum)beep=1;else{top+=key==25||key==32?13:8;if(top>maximum)top=maximum;}break;
    case 15:case 117:if(!top)beep=1;else{top-=key==15?13:8;if(top<0)top=0;}break;
    case 18:case 71:top=maximum;break;
    case 20:case 103:top=0;break;
    default:beep=1;break;
    }
    return uint16_t(top)|(close<<16)|(beep<<17)|(ret<<24);
}
extern "C" uint32_t cc_towers_markup(const uint8_t*p,uint32_t width,uint8_t*cells){
    unsigned n=0;bool bright=false;
    while(*p&&n<uint16_t(width)){
        unsigned color=bright?0x6f:0x67,ch=*p++;
        if(ch=='\\'){
            ch=*p++;if(!ch)break;
            if(ch=='('){bright=true;continue;}if(ch==')'){bright=false;continue;}
            if(ch!='\\')color=0x6e;
        }
        cells[n*2]=uint8_t(ch);cells[n*2+1]=uint8_t(color);++n;
    }
    return n;
}
extern "C" uint32_t cc_towers_format(const uint8_t*p,int32_t x,int32_t y){reset();format(unpack(p),x,y);return failed?2:0;}
extern "C" uint32_t cc_towers_group(uint8_t*p,uint32_t n){if(n>3855)return 2;for(unsigned i=0;i<n;i++){selected[i]=unpack(p+i*17);if(selected[i].type>=61)return 2;}group(n);for(unsigned i=0;i<n;i++)pack(p+i*17,selected[i]);return 0;}
extern "C" uint32_t cc_towers_generate(const uint8_t*f,uint32_t size,uint32_t at){
    reset();unsigned n=0;
    if((tu_mode?cc_airfield_analyze_file_tu:cc_airfield_analyze_file)(f,size,at))return 1;
    auto*summary=cc_airfield_summary_buffer();bool single=cc_airfield_summary_result()[2]!=0;
    for(unsigned t=0;t<64;t++){
        unsigned count=w(f+at+8);at+=16;starts[t]=n;counts[t]=count;if(n+count>9600)return 2;
        for(unsigned i=0;i<count;i++,at+=23){auto*p=f+at;unsigned packed=w(p);objects[n++]={int32_t(uint32_t(int32_t((t%8)*32-128+(packed&31)))*65536u+w(p+2)),int32_t(uint32_t(int32_t((t/8)*32-128+(packed>>5&31)))*65536u+w(p+4)),int32_t((packed>>10&31)*65536u+w(p+6)),uint16_t(w(p+21)^(cc_edition_is_other_worlds()?0:(w(p+2)^w(p+4)))),uint16_t(w(p+11)),0};}
    }
    if(single)for(unsigned i=0;i<4;i++)text("\f\r\n");
    for(unsigned region=single?4:0;region<(single?5:9);region++){
        unsigned count=0;
        for(unsigned t=0;t<64;t++){
            unsigned x=t%8,y=t/8;
            const unsigned band[8]={0,0,0,1,1,2,2,2};
            if(!single&&(band[x]!=region%3||band[y]!=region/3))continue;
            for(unsigned i=starts[t];i<starts[t]+counts[t];i++)if(objects[i].type<61&&(types[objects[i].type].flags&1)){
                if(count>=3855)return 2;
                selected[count++]=objects[i];
            }
        }
        text("\f\r\n\\(Objects near ");text(fieldFirst[region]);ch(' ');text(fieldLast[region]);text(":\\)\r\n\r\n");
        text("\\(Object                    Quality:  Range/Dir:    Alt:\\)\r\n");text("\\(                                     (Mi.)       (ft.)\\)\r\n");
        group(count);unsigned previous=0;
        for(unsigned i=0;i<count;i++){if((selected[i].group&31)!=previous)text("\r\n\r\n");format(selected[i],l(summary+region*14),l(summary+region*14+4));previous=selected[i].group&31;}
        text("\r\n\r\n");
    }
    return failed?2:0;
}

// TU.EXE4F69 uses the same grouping and text except its older spy description.
extern "C" uint32_t cc_tu_generate(const uint8_t*f,uint32_t size,uint32_t at){tu_mode=true;auto result=cc_towers_generate(f,size,at);tu_mode=false;return result;}
// TU3DEF uses a22-row body,20-row pages and11-row half-pages; its older
// dispatcher has no I/B/N exit aliases from MOAG's later mode7 reader.
extern "C" uint32_t cc_tu_view_key(uint32_t t,uint32_t n,uint32_t key){
    int top=int16_t(t),maximum=int16_t(n)>22?int16_t(n)-22:0;unsigned beep=0,close=0,ret=0;
    switch(uint16_t(key)){
    case 8:case 10:case 27:case 113:close=1;ret=key==10?1:0;break;
    case 6:case 14:case 50:case 54:case 106:if(++top>maximum){beep=1;top=maximum;}break;
    case 2:case 16:case 52:case 56:case 107:if(--top<0){beep=1;top=0;}break;
    case 25:case 32:case 100:if(top==maximum)beep=1;else{top+=key==25||key==32?20:11;if(top>maximum)top=maximum;}break;
    case 15:case 117:if(!top)beep=1;else{top-=key==15?20:11;if(top<0)top=0;}break;
    case 18:case 71:top=maximum;break;
    case 20:case 103:top=0;break;
    default:beep=1;break;
    }
    return uint16_t(top)|(close<<16)|(beep<<17)|(ret<<24);
}
