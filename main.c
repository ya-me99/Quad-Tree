#include "stb_image.h"
#include "stdlib.h"
#include "stdio.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stbi_image_write.h"

/*
 *  Writeing  a Quadtree Generator  and Tests
 *  Letz start by loading a image with alpha values
 *  Next Write Basic Funtions to put out Quadtree
 *  Build Actuall Data Structure 
 *  Optimize Multithreaded SIMD
 *  Write Test Stuff 
 *  Be Happy
*/


typedef struct
{ 
 uint8_t* img;
 uint64_t w,h;
 uint64_t bytes_per_line;
 uint64_t bytes;
 uint8_t ch;
}Image;

typedef struct
{
 Image img;
 uint32_t* pairs;
}Shape;

typedef struct
{
 uint64_t xt,xb,yt,yb;
 uint64_t w,h;
 uint64_t bytes_per_line;
 uint64_t bytes;
 uint8_t ch;
 uint8_t* img;
 uint8_t is_alpha;
 uint8_t is_color;
}Quad;

typedef struct 
{
 Quad* quads;
 uint8_t lock;
 uint64_t next;
 uint64_t cnt;
}QuadTree;

typedef struct
{
 uint8_t* map;
 uint16_t type;
}PixMap;

typedef struct
{
 uint8_t* img;
 uint64_t w,h;
 uint64_t bytes_per_line;
 uint64_t bytes;
 uint8_t  ch;
 PixMap   pix;   // Direct Map 
 QuadTree qt1;
}QImage;

QImage  QImageInit(uint8_t* img, uint64_t x, uint64_t y , uint8_t ch);
uint8_t QImage_PixMap(QImage* qi);
uint8_t QImage_QuadTreeT1(QImage* qi);
uint8_t QImage_SplitQuad(QImage* qi, Quad q);
uint8_t QImage_BlitQuad(QImage qi,Quad q);
uint8_t Quad_Image(QImage qi,Quad *q);
uint8_t Quad_Fill(Quad q,uint8_t pat[4]);
uint8_t Quad_IsAlpha(Quad q);
uint8_t Quad_Border(uint8_t pat[4], Quad q);
uint8_t Quad_IsAlphaOrColor(Quad q);
uint8_t Shape_FindPairs(Shape* s);
void Shape_Fill(Shape s);

// Bezier

uint64_t screen_width;
uint64_t screen_height;


uint8_t  Casteljau(float* pts , uint64_t count,
                   uint64_t t , float scale , Image* image);

uint8_t Bernstein(float* pts , uint64_t count,
                    uint64_t t , float scale, Image* image);

uint64_t Count;

int main(){

 printf(" Quad Tree ");  
 Count=0;
 screen_width=1920;
 screen_height=1080;
 
 int x,y,n;
 uint64_t img_size; 

 uint8_t *img=stbi_load("test.png",&x,&y,&n,0);

 if(img==NULL){ printf(" Cant Open Image "); }
 
 if(n!=4) { printf(" No Alpha Value \n "); exit(1); }

 QImage qi=QImageInit(img,x,y,n);

 QImage_QuadTreeT1(&qi);
 
 uint8_t pat[4]={0,255,0,255};

 for(uint64_t i=0;i<qi.qt1.next;i++)
 {
  Quad_Border(pat,qi.qt1.quads[i]);
  QImage_BlitQuad(qi,qi.qt1.quads[i]);
 }
  
 Quad q=qi.qt1.quads[5];

 stbi_write_bmp("ImgOut.bmp",qi.w,qi.h,4,qi.img);
 stbi_write_bmp("Quad.bmp",q.w,q.h,4,q.img);
 

//printf(" Count %lu \n ", Count );
 return 0;
}


QImage QImageInit(uint8_t* img, uint64_t x, uint64_t y , uint8_t ch)
{
 
 QImage qi={ };

 qi.img=img;
 qi.w=x;
 qi.h=y;
 qi.ch=ch;
 qi.bytes_per_line=qi.w*qi.ch;
 qi.bytes=qi.bytes_per_line*qi.h;

 return qi;
}

// Todo: Use Bits Not Bytes 

uint8_t QImage_PixMap(QImage* qi)
{
 
 uint64_t QuadCount=qi->w*qi->h;            
 
 qi->pix.map=calloc(1,QuadCount);

 if(1){ // type==PIX_ALPHA
  
  for(uint64_t i=0;i<QuadCount;i+=4){
   
    if(!qi->img[i+3]==0) { qi->pix.map[i]=1; };
 
  }
 
 }
}


uint8_t QImage_QuadTreeT1(QImage* qi)
{
 
 // Make As Many Quads as There are Pixels to Later Resize it
 
 qi->qt1.quads=calloc(sizeof(Quad),qi->bytes);

 Quad mq;
 mq.xt=0;
 mq.xb=qi->w;
 mq.yt=0;
 mq.yb=qi->h;
 mq.w=qi->w;
 mq.h=qi->h;
 mq.bytes_per_line=qi->bytes_per_line;
 mq.bytes=qi->bytes;
 mq.ch=4;
 

 QImage_SplitQuad(qi,mq);

 uint64_t q_num=0;
 uint64_t last=0;
 uint8_t state=0;

 while(1)
 {
  
  Quad q=qi->qt1.quads[q_num];
  uint64_t limit=q.yb-q.yt;
  printf(" %d , %d , %d \n ",q_num,limit,qi->qt1.next);
  
  if(limit==1){ qi->qt1.cnt=qi->qt1.next; break; } 
 
  switch(Quad_IsAlphaOrColor(q)){
   case 0:QImage_SplitQuad(qi,qi->qt1.quads[q_num]);q_num++; printf("1 \n");break; 
   case 1:QImage_SplitQuad(qi,qi->qt1.quads[q_num]);q_num++; printf("2 \n");break; 
   case 2:qi->qt1.quads[q_num].is_alpha=1;q_num++;  printf("3 \n");break;
   case 3:qi->qt1.quads[q_num].is_color=1;q_num++;  printf("4 \n");break;
   default: printf(" WHAT DA FUQ HAPPEND  \n "); break ;
  }
 }
  
 printf(" Build Quad Tree With %d Quads",qi->qt1.cnt);
}


uint8_t QImage_SplitQuad(QImage* qi, Quad q)
{
 uint8_t pat[4]={0,255,0,255};  
 
 Quad q1,q2,q3,q4 = { };

 // Top Left Split 

 q1.xt=q.xt;
 q1.xb=q.xt+q.w/2;
 q1.yt=q.yt;
 q1.yb=q.yt+q.h/2;
 q1.ch=qi->ch;
 
 q1.w=q1.xb-q1.xt;
 q1.h=q1.yb-q1.yt;
 q1.bytes_per_line=q1.w*q1.ch;
 q1.bytes=q1.bytes_per_line*q1.h;

 Quad_Image(*qi,&q1);
 qi->qt1.quads[qi->qt1.next]=q1;
 qi->qt1.next++;

 // Top Right Split 
 q2.xt=q.xt+q.w/2;
 q2.xb=q.xb;
 q2.yt=q.yt;
 q2.yb=q.yt+q.h/2;
 q2.ch=qi->ch;  

 q2.w=q2.xb-q2.xt;
 q2.h=q2.yb-q2.yt;
 q2.bytes_per_line=q2.w*q2.ch;
 q2.bytes=q2.bytes_per_line*q2.h;

 Quad_Image(*qi,&q2); 
 qi->qt1.quads[qi->qt1.next]=q2;
 qi->qt1.next++;

 
 // Top Left Split 
 q3.xt=q.xt;
 q3.xb=q.xt+(q.w)/2;
 q3.yt=q.yt+(q.h)/2;
 q3.yb=q.yb;
 q3.ch=qi->ch;  

 q3.w=q3.xb-q3.xt;
 q3.h=q3.yb-q3.yt; 
 q3.bytes_per_line=q3.w*q3.ch;
 q3.bytes=q3.bytes_per_line*q3.h;
 
 Quad_Image(*qi,&q3);  
 qi->qt1.quads[qi->qt1.next]=q3;
 qi->qt1.next++;
  

 // Top Right Split 
 q4.xt=q.xt+(q.w)/2;
 q4.xb=q.xb;
 q4.yt=q.yt+(q.h)/2;
 q4.yb=q.yb;
 q4.ch=qi->ch;  

 q4.w=q4.xb-q4.xt;
 q4.h=q4.yb-q4.yt; 
 q4.bytes_per_line=q4.w*q4.ch;
 q4.bytes=q4.bytes_per_line*q4.h;
 

 Quad_Image(*qi,&q4);
 qi->qt1.quads[qi->qt1.next]=q4;
 qi->qt1.next++;

 return 1;
}

uint8_t QImage_BlitQuad(QImage qi,Quad q)
{

 for(uint64_t i=0;i<q.h;i++)
 {
  uint64_t off1=( (q.yt+i)*qi.bytes_per_line ) + ( q.xt*q.ch );
  uint64_t off2=i*q.bytes_per_line;

  memcpy(&(qi.img[off1]),&(q.img[off2]),q.bytes_per_line);
 }
 
}

uint8_t Quad_Fill(Quad q, uint8_t pat[4])
{
 printf(" bytes \n ", q.bytes);
 printf(" pat  %d %d %d %d \n ", pat[0],pat[1],pat[2],pat[3]);
 
 for(uint64_t i=0;i<q.bytes;i+=4)
 {
  q.img[i]=pat[0];
  q.img[i+1]=pat[1];
  q.img[i+2]=pat[2];
  q.img[i+3]=pat[3];
 }

}


uint8_t Quad_Image(QImage qi,Quad *q){

 q->img=malloc(q->bytes);

 for(uint64_t i=0;i<q->h;i++){

  uint64_t off1=i*q->bytes_per_line;
  uint64_t off2=(q->yt+i)*qi.bytes_per_line+(q->xt*4);

  memcpy(&(q->img[off1]),&(qi.img[off2]),q->bytes_per_line);
 }

}


uint8_t Quad_IsAlpha(Quad q){

 for(uint64_t i=3;i<q.bytes;i+=4){
  
  if(q.img[i]==255){ printf(" Isnt \n "); return 0 ; }

 }
 
 printf(" Is \n ");
 return 1;
}


uint8_t Quad_IsAlphaOrColor(Quad q){

  if(q.img[3]==0){
    for(uint64_t i=3;i<q.bytes;i+=4){
      if(!q.img[i]==0){ printf(" Not Pure Transparent \n "); return 0 ; }
     }
    return 2;
  }else{
    for(uint64_t i=3;i<q.bytes;i+=4){
      if(q.img[i]==0){ printf(" Not Pure Visible \n "); return 1 ; }
     }
     return 3;
  }

}



uint8_t Quad_Border(uint8_t pat[4], Quad q)
{

 for(uint64_t i=0;i<q.bytes_per_line;i+=4){
  
  q.img[i]=pat[0];
  q.img[i+1]=pat[1];
  q.img[i+2]=pat[2];
  q.img[i+3]=pat[3];

 }

 uint64_t offset=q.bytes_per_line*(q.h-1);

 for(uint64_t i=offset;i<q.bytes;i+=4){
  
  q.img[i]=pat[0];
  q.img[i+1]=pat[1];
  q.img[i+2]=pat[2];
  q.img[i+3]=pat[3]; 
 
 }

 for(uint64_t i=1;i<q.h-1;i++){

  uint64_t s=i*q.bytes_per_line;
  
  q.img[s]=pat[0];
  q.img[s+1]=pat[1];
  q.img[s+2]=pat[2];
  q.img[s+3]=pat[3];
 
  q.img[s-1]=pat[3];
  q.img[s-2]=pat[2];
  q.img[s-3]=pat[1];
  q.img[s-4]=pat[0];
 }
 
 return 0;
}



// ------------------------------------ Shape ---------------------------------

 /* float test[24]={0.25,0.25,0.50,0.0,0.75,0.25,  
                 0.75,0.25,1.0,0.5,0.75,0.75,
                 0.75,0.75,0.50,1.0,0.25,0.75,
                 0.25,0.75,0.0,0.50,0.25,0.25 };

 Shape s={ };

 for(uint64_t i=0;i<10000;i++){
  Bernstein(test,4,1000,1,&(s.img));
 }

 for(uint64_t i=0;i<10000;i++){
  Casteljau(test,4,1000,1,&(s.img));
 }

 for(uint64_t i=0;i<10000;i++){
  Shape_FindPairs(&s);
 }

 for(uint64_t i=0;i<10000;i++){
  Shape_Fill(s);
 }

 Shape_Fill(s);
 */

//vec2 pos=(1-t)*(1-t)*Cr_Pts[0]+2*t*(1-t)*Cr_Pts[1]+(t*t)*Cr_Pts[2]; @

uint8_t Bernstein(float* pts , uint64_t count,
                 uint64_t t , float scale, Image *image){
 
 uint64_t size_w=screen_width*scale;
 uint64_t size_h=screen_height*scale;
 uint64_t size_bytes=size_w*size_h*4;
  
 if(image->img==NULL)
 {
  image->img=calloc(1,size_bytes);
 }
 
 uint8_t* img=image->img;

 for(uint64_t n=0;n<count*6;n+=6){

   float tt=0;
   float step=1 / ( (float)t );
  
  for(uint64_t i=0;i<t;i++){
   
   tt+=step;

   float pos_x=(1-tt)*(1-tt)*pts[n]+2*tt*(1-tt)*pts[n+2]+tt*tt*pts[n+4]; 
   float pos_y=(1-tt)*(1-tt)*pts[n+1]+2*tt*(1-tt)*pts[n+3]+tt*tt*pts[n+5]; 
   

   uint64_t f_pos_x=pos_x*screen_width;
   uint64_t f_pos_y=pos_y*screen_height;

   uint64_t img_pos=f_pos_y*size_w*4+f_pos_x*4;

   img[img_pos]=255;
   img[img_pos+1]=0;
   img[img_pos+2]=0; 
   img[img_pos+3]=255;
  }
}
 
 image->img=img;
 image->w=size_w;
 image->h=size_h;
 image->bytes=size_bytes;
 image->bytes_per_line=size_w*4;
 image->ch=4;

 return 1; 
}

uint8_t Casteljau(float* pts , uint64_t count,
                  uint64_t t , float scale , Image *image)
{
 uint64_t size_w=screen_width*scale;
 uint64_t size_h=screen_height*scale;
 uint64_t size_bytes=size_w*size_h*4;

 if(image->img==NULL)
 {
  image->img=calloc(1,size_bytes);
 }
 
 uint8_t* img=image->img;

 // p1*(1-t)+p2*t Lerp

 for(uint64_t n=0;n<count*6;n+=6){
  float tt=0;
  float step=1 / ( (float)t );

  for(uint64_t i=0;i<t;i++){
   tt+=step;
  
   float p1x=pts[n];
   float p2x=pts[n+2];
   float p3x=pts[n+4];

   float p1y=pts[n+1];
   float p2y=pts[n+3];
   float p3y=pts[n+5];

   float p4x=p1x*(1-tt)+p2x*tt;
   float p5x=p2x*(1-tt)+p3x*tt;
   float p6x=p4x*(1-tt)+p5x*tt;

   float p4y=p1y*(1-tt)+p2y*tt;
   float p5y=p2y*(1-tt)+p3y*tt;
   float p6y=p4y*(1-tt)+p5y*tt;
  
   uint64_t final_x=p6x*screen_width;
   uint64_t final_y=p6y*screen_height;

   uint64_t img_pos=final_y*screen_width*4+final_x*4;

   img[img_pos]=255;
   img[img_pos+1]=0;
   img[img_pos+2]=0; 
   img[img_pos+3]=255; 
  }
 }
 
 image->img=img;
 image->w=size_w;
 image->h=size_h;
 image->bytes=size_bytes;
 image->bytes_per_line=size_w*4;
 image->ch=4;

 return 1;
}


uint8_t Shape_FindPairs(Shape* s)
{
 if(s->img.img==NULL){ exit(1); } 
 

 if(s->pairs==NULL){
  s->pairs=calloc(1,s->img.h*20);
 }
 
 uint32_t* pairs=s->pairs;

 uint64_t p_cnt=0;

 int32_t a=-1;
 
 int32_t b=-1;
 
 uint8_t* img=s->img.img;

 for(uint64_t i=0;i<s->img.h;i++){
  
  for(uint64_t j=0;j<s->img.bytes_per_line;j+=4){
   Count++;
   uint64_t pos=i*s->img.bytes_per_line+j;

   if(img[pos]==255){
    if(a==-1){
      a=pos;
     }
     else{
      b=pos;
     }
    }
  }
  
  if(b!=-1 && a!=-1){
   pairs[p_cnt]=a; 
   pairs[p_cnt+1]=b;
   p_cnt+=2;
  }
   a=-1;
   b=-1;
 }

 s->pairs=pairs;
 return 1;
}

void Shape_Fill(Shape s){

 if(s.pairs==NULL){ printf("Init Pairs First"); return ; }
 
 uint64_t p=0;

 for(uint64_t i=0;i<s.img.h;i++)
 {
  uint64_t start=s.pairs[p];
  uint64_t end=s.pairs[p+1];
 
  memset( &(s.img.img[start]) ,255, end-start );
  p+=2;
 }
}
