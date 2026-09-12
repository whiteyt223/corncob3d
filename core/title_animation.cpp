#include "title_animation.hpp"
// Original CCTITLE.EXE main image49BB..55D9 and graphics helpers3441,
// 33C4,33EA,32E4,36A3,5866,58B4,5920,59AB,59FF,5A2E.
// One call advances one source loop iteration. The original has no frame timer.
namespace {
uint8_t data[300000],video[262144],dac[768],pixels[64000],events[1024];
bool other_worlds=false;
uint16_t locals[8]; // phase, fade brightness, credits toggle, music bits, last key, displayed offset, event count, reserved
int s(int v){v&=65535;return v<32768?v:v-65536;}
uint16_t w(unsigned a){a&=65535;return uint16_t(data[a]|unsigned(data[(a+1)&65535])<<8);}
int v(unsigned a){return s(w(a));}
void sw(unsigned a,int n){a&=65535;data[a]=uint8_t(n);data[(a+1)&65535]=uint8_t(unsigned(n)>>8);}
uint32_t dw(unsigned a){return uint32_t(w(a))|(uint32_t(w(a+2))<<16);}
void sd(unsigned a,uint32_t n){sw(a,int(n));sw(a+2,int(n>>16));}
void add(unsigned a,int n){sw(a,v(a)+n);}
void event(unsigned fn,int a=0,int b=0){unsigned n=locals[6]++;if(n>=128)return;unsigned p=n*8;events[p]=uint8_t(fn);events[p+1]=uint8_t(fn>>8);events[p+2]=uint8_t(a);events[p+3]=uint8_t(unsigned(a)>>8);events[p+4]=uint8_t(b);events[p+5]=uint8_t(unsigned(b)>>8);events[p+6]=events[p+7]=0;}
int random(){uint32_t seed=dw(0x696)*0x015a4e35u+1;sd(0x696,seed);return int((seed>>16)&32767);}
void clear(){for(unsigned i=0;i<sizeof video;++i)video[i]=0;}
void palette(unsigned at){for(unsigned i=0;i<768;++i)dac[i]=data[(at+i)&65535]&63;}
void fade_air(){unsigned den=w(0x1ca6)*7u;for(unsigned i=0;i<96;++i)dac[576+i]=uint8_t((unsigned(data[0x1a02+i])*locals[1]/den)&63);}
void level(int delta){int n=s(int(locals[1])+delta);if(n<0)n=0;int max=s(v(0x1ca6)*7-1);if(max<n)n=max;locals[1]=uint16_t(n);fade_air();}
void copyrow(int n,int dst,int src){for(int i=0;i<n;++i)for(unsigned p=0;p<4;++p)video[p*65536+uint16_t(dst+i)]=video[p*65536+uint16_t(src+i)];}
void row4(int n,int dst,uint32_t src,int mode=0,int limit=255){
 for(unsigned p=0;p<4;++p)for(int i=0;i<n;++i){uint8_t b=data[src++];unsigned q=p*65536+uint16_t(dst+i);if(mode){if(mode==2&&b>=uint8_t(limit))b=uint8_t(limit);b=uint8_t((b&224)|(video[q]&31));}video[q]=b;}
}
void sprite(int x,int y,int width,int height,unsigned table){
 // The source computes a right-clipped width but never uses it afterward.
 (void)width;int lines,source_y=0,dest_y=y;
 if(y<0){lines=s(y+height);if(lines<0)lines=0;source_y=-y;dest_y=0;}
 else{lines=s(200-y);if(lines>height)lines=height;}
 for(int j=0;j<lines;++j){unsigned row=uint16_t(table+uint16_t(source_y+j)*4),ptr=w(row+2);for(int k=0;k<v(row);++k){unsigned r=w(ptr+k*2),plane=w(r);if(plane>3)continue;unsigned src=dw(r+6);int dst=s(v(r+2)+s((dest_y+j)*80)+x/4+v(0x27ca));for(int i=0;i<v(r+4);++i)video[plane*65536+uint16_t(dst+i)]=data[src+i];}}
}
void flip(){uint16_t n=w(0x27cc);sw(0x27cc,w(0x27ca));sw(0x27ca,n);locals[5]=w(0x27cc);}
int reflected(unsigned scroll,int line,int height){
 if(v(scroll)<0)sw(scroll,s(height*2-1));
 int n=s(v(scroll)+line);
 if(n<height)return n;
 if(s(height*2)>n)return s(height*2-v(scroll)-line-1);
 if(s(height*2)>v(scroll))return s(n-s(height*2));
 sw(scroll,0);return line;
}
void aircraft_position(){
 add(0x27f8,-v(0x27be));if(s(-v(0x27fc)-s(v(0x27be)*90))>v(0x27f8)){
  sw(0x27f8,s(v(0x27be)*90+210));if(data[0xd0]){sw(0xe9e,255);data[0xd0]=0;}
  else if(v(0x127a)<2)add(0x127a,1);else sw(0xe9e,255);
 }
}
void aircraft_audio(int x,int size){
 x=s(size/2+s(x-size/2)/2);int pitch;
 if(x<0)pitch=524;else if(x>size)pitch=500;else pitch=s(524-(int32_t(x)*24/size));event(0x3d0,0,pitch);
 int dist=s(size/2-x);if(dist<0)dist=s(-dist);int gain=0;
 if(size/2<=dist&&s(size+s(size*2)/3)>dist)gain=s(s(s(dist-size/2)*63)/(s(size*2)/3));
 gain=s(gain+10);if(gain<0)gain=0;if(gain>63)gain=63;if(!w(0x1278))gain=63;event(0x447,67,gain);event(0x447,68,gain);
}
bool key(int input){
 if(input<0)return false;
 locals[4]=uint8_t(input);unsigned k=locals[4];
 if(k==27)return true;
 if(k==45){add(0x27be,-1);if(v(0x27be)<1)sw(0x27be,1);}
 else if(k==43){add(0x27be,1);if(v(0x27be)>150)sw(0x27be,150);}
 else if(k==81||k==113)sw(0x1278,w(0x1278)^65535);
 else if(k==108||(!locals[0]&&data[0xd0])){locals[2]^=255;sw(0xe9c,224);sw(0xe9e,255);locals[4]=0;}
 else{locals[4]=27;return true;}return false;
}
void begin_air(){
 locals[0]=0;clear();palette(0x16a2);sw(0xe9e,0);locals[1]=0;fade_air();sw(0x27f2,4);sw(0x27f0,12);sw(0x1288,0);sw(0x127a,0);
 for(int i=0;i<v(0x2800);++i)row4(v(0x2802)/4,s(v(0x27c8)+i*80),dw(0x1fce + i*4));
 event(0x422);event(0x447,64,16);event(0x447,75,63);event(0x447,163,50);event(0x447,69,10);if(!w(0x1278))event(0x447,69,63);event(0x3d0,3,50);sw(0x27d4,0);if(locals[4]!=27)locals[4]=0;
}
void begin_logo(){
 locals[0]=1;event(0x3f9,3,50);event(0x3f9,0,512);clear();palette(0x2c00); // Source GRADSKY.PAL stored in reserved normalized data area.
 sw(0x27ce,0);locals[2]=255;sw(0xe9e,65535);sw(0xe9c,32);sw(0x1288,0);dac[21]=dac[22]=dac[23]=0;
 for(int i=0;i<v(0x27d0);++i)row4(v(0x27d2)/4,s(v(0x27c8)+i*80),dw(0x22ee + i*4));
 for(int i=0;i<v(0x27dc);++i)row4(v(0x27de)/4,s(0xc000+i*80),dw(0x1cae + i*4));
 if(locals[3]&2){event(0x5bfb);locals[3]|=1;}
}
void finish(){
 if(locals[3]&2){if(locals[3]&1)event(0x5c65);locals[3]&=65534;}
 if(locals[4]!=27){begin_air();return;}locals[0]=2;clear();event(0x3f9,0,512);event(0x447,67,63);sw(0x1282,0);sw(0x1280,0);
}
void step_air(int input){
 add(0x1288,1);flip();add(0x27d4,-1);
 for(int i=0;i<v(0x2800);++i){int row=reflected(0x27d4,i,v(0x2800));copyrow(v(0x2802)/4,s(v(0x27ca)+i*80),s(v(0x27c8)+row*80));}
 if(!w(0xe9e)&&v(0x1288)>5){
  sw(0x792,s(v(0x792)+1)%4);unsigned i=uint16_t(w(0x792)*4);sw(0x794,s(s(v(0x27f8)-150)*2+v(0xc02+i)/2));
  if(v(0x794)<-200){int r=random()>>8;sw(0x796,s(160-v(0xc00+i)/2+r-64));}if(v(0x796)<160)add(0x796,1);if(v(0x796)>160)add(0x796,-1);
  sw(0x790,s(v(0x794)-100));if(v(0x790)<0)sw(0x790,-v(0x790));sw(0x790,v(0x790)/20);if(v(0x790)>63||!w(0x1278))sw(0x790,63);if(v(0x790)<12)sw(0x790,12);
  event(0x447,75,v(0x790));event(0x447,163,50);sprite(v(0x796),v(0x794),v(0xc00+i),v(0xc02+i),w(0xbe + w(0x792)*2));
 }
 if((random()>>9)+10<v(0x798))sw(0x798,0);
 if(!w(0xe9e)&&v(0x798)<5){
  sw(0x79e,v(0xc6+w(0x798)*2)%3);if(!w(0x79e)){sw(0x79c,(random()>>7)+32);sw(0x79a,(random()>>8)+36);event(0x3f9,2,0);event(0x3d0,2,0);}else add(0x79a,11);
  unsigned i=w(0x79e)*4;sw(0x7a2,s(v(0x79c)-v(0xc10+i)/2));sw(0x7a0,s(v(0x79a)-v(0xc12+i)/2));
  sprite(v(0x7a2),v(0x7a0),v(0xc10+i),v(0xc12+i),w(0xb8+w(0x79e)*2));if(w(0x79e)==2)locals[4]=0;
 }
 add(0x798,1);int max=s(v(0x1ca6)*7-1);if(!w(0xe9e)&&max>s(locals[1]))level(10);if(w(0xe9e)==255&&s(locals[1])>0)level(-10);
 if(locals[4]!=27&&max==s(locals[1])){
  sprite(v(0x27fa),v(0x27f8),v(0x27fe),v(0x27fc),0x108c);aircraft_position();
  if(v(0x27f8)>-50&&v(0x27f8)<160&&v(0x27d4)%2==0){
   sw(0x27f0,v(0x27f8)+22);sw(0x27f2,v(0x27fa)+52);sprite(v(0x27f2),v(0x27f0),v(0x27f6),v(0x27f4),0xea0);
   sw(0x27f0,v(0x27f8)+22);sw(0x27f2,v(0x27fa)+116);sprite(v(0x27f2),v(0x27f0),v(0x27f6),v(0x27f4),0xea0);event(0x3f9,1,512);event(0x3d0,1,512);
  }
 }else sw(0x27f8,s(v(0x27be)*30+210));
 aircraft_audio(s(s(v(0x2800)*2)/3-s(v(0x27f8)+v(0x27fc)/2)),v(0x2800));
 if(key(input)||(w(0xe9e)==255&&!locals[1])){begin_logo();if(locals[4]==27)finish();}
}
void logo_rows(int width,int height,int x,int y,unsigned table,bool bottom){
 for(int i=0;i<height;++i){int relative=bottom?s(i+y-v(0x27e8)):i;int row=reflected(0x27ce,relative,v(0x27d0));
  int src=s(v(0x27c8)+row*80+(bottom?x/4-v(0x27ea)/4:0)),dst=s(v(0x27ca)+s(i+y)*80+x/4);copyrow(width/4,dst,src);
  row4(width/4,dst,dw(table+i*4),w(0xe9e)?2:1,v(0xe9c));
 }
}
void step_logo(int input,bool playing){
 add(0x1288,1);flip();if((locals[3]&3)==3&&!playing){event(0x5c65);locals[3]&=65534;}
 if(other_worlds){add(0x27ce,1);logo_rows(v(0x27e6),v(0x27e4),v(0x27e2),v(0x27e0),0x252e,false);}
 else{add(0x27ce,-1);logo_rows(v(0x27ee),v(0x27ec),v(0x27ea),v(0x27e8),0x264e,false);logo_rows(v(0x27e6),v(0x27e4),v(0x27e2),v(0x27e0),0x252e,true);}
 if(w(0xe9e)==65535){if(v(0xe9c)<224){add(0xe9c,32);dac[21]=dac[22]=dac[23]=uint8_t(s(v(0xe9c)*42)/224)&63;}else sw(0xe9e,0);}
 if(w(0xe9e)==255){if(v(0xe9c)>32){add(0xe9c,-32);dac[21]=dac[22]=dac[23]=uint8_t(s(v(0xe9c)*42)/224)&63;}else{finish();return;}}
 if(locals[2]){
  for(int i=0;i<s(199-v(0x27d8));++i){int row=s(i+v(0x27d6));if(row>=v(0x27dc))row=s(row-v(0x27dc));copyrow(v(0x27de)/4,s(v(0x27ca)+s(i+v(0x27d8))*80+v(0x27da)/4),s(0xc000+row*80));}
  add(0x27d6,1);if(!(locals[3]&2)){if(v(0x27d6)>=v(0x27dc)){sw(0x27d6,0);sw(0xe9e,255);}}
  else{if(!(locals[3]&1))sw(0xe9e,255);if(v(0x27d6)>=v(0x27dc))sw(0x27d6,0);}
 }
 if(key(input))finish();
}
}
extern "C" uint8_t* cc_title_data(){return data;}
extern "C" uint32_t cc_title_data_capacity(){return sizeof data;}
extern "C" void cc_title_init_profile(uint32_t profile,uint32_t music,uint32_t sound,uint32_t seed){other_worlds=profile==2;for(unsigned i=0;i<8;++i)locals[i]=0;locals[3]=music?2:0;sw(0x1278,sound?65535:0);sd(0x696,seed);begin_air();}
extern "C" uint32_t cc_title_step(int32_t key,uint32_t playing){locals[6]=0;if(locals[0]==0)step_air(key);else if(locals[0]==1)step_logo(key,playing!=0);return locals[0];}
extern "C" uint32_t cc_title_phase(){return locals[0];}
extern "C" uint8_t* cc_title_pixels(){for(unsigned y=0;y<200;++y)for(unsigned x=0;x<320;++x)pixels[y*320+x]=video[(x&3)*65536+uint16_t(locals[5]+y*80+x/4)];return pixels;}
extern "C" uint8_t* cc_title_palette(){return dac;}
extern "C" uint8_t* cc_title_video(){return video;}
extern "C" uint8_t* cc_title_events(){return events;}
extern "C" uint32_t cc_title_event_count(){return locals[6];}
extern "C" uint16_t* cc_title_locals(){return locals;}

extern "C" void cc_title_init(uint32_t music,uint32_t sound,uint32_t seed){cc_title_init_profile(0,music,sound,seed);}
