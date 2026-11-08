/* Independent H264 interpolation reference, shared by host/PowerPC checks. */
static int clip_byte(int v){return v<0?0:v>255?255:v;}
static int tap6(const uint8_t *p,ptrdiff_t s){return p[-2*s]-5*p[-s]+20*p[0]+20*p[s]-5*p[2*s]+p[3*s];}
static int half_h(const uint8_t *p){return clip_byte((tap6(p,1)+16)>>5);}
static int half_v(const uint8_t *p,ptrdiff_t s){return clip_byte((tap6(p,s)+16)>>5);}
static int centre(const uint8_t *p,ptrdiff_t s){return clip_byte((tap6(p-2*s,1)-5*tap6(p-s,1)+20*tap6(p,1)+20*tap6(p+s,1)-5*tap6(p+2*s,1)+tap6(p+3*s,1)+512)>>10);}
static void scalar_qpel(uint8_t *d,const uint8_t *src,ptrdiff_t stride,int size,int avg,int pos){
 int x=pos&3,y=pos>>2;
 for(int j=0;j<size;j++)for(int i=0;i<size;i++){
  const uint8_t *s=src+j*stride+i;int v;
  if(!(x&1)&&!(y&1))v=x?(y?centre(s,stride):half_h(s)):(y?half_v(s,stride):s[0]);
  else if((x&1)&&(y&1))v=(half_h(s+(y==3?stride:0))+half_v(s+(x==3),stride)+1)>>1;
  else if(x&1)v=y?(half_v(s+(x==3),stride)+centre(s,stride)+1)>>1:(s[x==3]+half_h(s)+1)>>1;
  else v=x?(half_h(s+(y==3?stride:0))+centre(s,stride)+1)>>1:(s[y==3?stride:0]+half_v(s,stride)+1)>>1;
  unsigned at=j*stride+i;d[at]=(uint8_t)(avg?(d[at]+v+1)>>1:v);
 }
}
static void scalar_chroma(uint8_t *d,uint8_t *s,int stride,int h,int x,int y,int width,int avg){
 for(int j=0;j<h;j++)for(int i=0;i<width;i++){
  unsigned at=j*stride+i;int v=((8-x)*(8-y)*s[at]+x*(8-y)*s[at+1]+(8-x)*y*s[at+stride]+x*y*s[at+stride+1]+32)>>6;
  d[at]=(uint8_t)(avg?(d[at]+v+1)>>1:v);
 }
}
#define SMC(S,P) static void put##S##_##P(uint8_t *d,const uint8_t *s,ptrdiff_t t){scalar_qpel(d,s,t,S,0,P);} static void avg##S##_##P(uint8_t *d,const uint8_t *s,ptrdiff_t t){scalar_qpel(d,s,t,S,1,P);}
#define SALL(S) SMC(S,0) SMC(S,1) SMC(S,2) SMC(S,3) SMC(S,4) SMC(S,5) SMC(S,6) SMC(S,7) SMC(S,8) SMC(S,9) SMC(S,10) SMC(S,11) SMC(S,12) SMC(S,13) SMC(S,14) SMC(S,15)
SALL(16) SALL(8)
#define STAB(O,S) {O##S##_0,O##S##_1,O##S##_2,O##S##_3,O##S##_4,O##S##_5,O##S##_6,O##S##_7,O##S##_8,O##S##_9,O##S##_10,O##S##_11,O##S##_12,O##S##_13,O##S##_14,O##S##_15}
static void scalar_qpel_init(H264QpelContext *c){qpel_mc_func p[2][16]={STAB(put,16),STAB(put,8)},a[2][16]={STAB(avg,16),STAB(avg,8)};for(int s=0;s<2;s++)for(int i=0;i<16;i++){c->put_h264_qpel_pixels_tab[s][i]=p[s][i];c->avg_h264_qpel_pixels_tab[s][i]=a[s][i];}}
#define SCH(W) static void putc##W(uint8_t *d,uint8_t *s,int t,int h,int x,int y){scalar_chroma(d,s,t,h,x,y,W,0);} static void avgc##W(uint8_t *d,uint8_t *s,int t,int h,int x,int y){scalar_chroma(d,s,t,h,x,y,W,1);}
SCH(8) SCH(4)
static void scalar_chroma_init(H264ChromaContext *c){c->put_h264_chroma_pixels_tab[0]=putc8;c->put_h264_chroma_pixels_tab[1]=putc4;c->avg_h264_chroma_pixels_tab[0]=avgc8;c->avg_h264_chroma_pixels_tab[1]=avgc4;}
