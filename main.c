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


#define SET_QUAD_ALPHA(a,n) a || (uint8_t)1<<n*2
#define SET_QUAD_COLOR(a,n) a || (uint8_t)2<<n*2


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
 Quad* quads;
 uint8_t lock;
 uint64_t next;
 uint64_t cnt;
}QuadTreeRT;

typedef struct
{
 uint8_t* img;
 uint64_t w,h;
 uint64_t bytes_per_line;
 uint64_t bytes;
 uint8_t  ch;
 QuadTree qt_dbg;
 QuadTree qt_rt;
 Quad* qp;
 uint64_t qp_cnt;
}QImage;

QImage  QImageInit(uint8_t* img, uint64_t x, uint64_t y , uint8_t ch);
uint8_t QImage_QuadTreeT1(QImage* qi);
uint8_t QImage_SplitQuad(QImage* qi, Quad q);
uint8_t QImage_BlitQuad(QImage qi,Quad q);
uint8_t Quad_Image(QImage qi,Quad *q);
uint8_t Quad_Fill(Quad q,uint8_t pat[4]);
uint8_t Quad_IsAlpha(Quad q);
uint8_t Quad_Border(uint8_t pat[4], Quad q);
uint8_t Quad_IsAlphaOrColor(Quad q);

// Bezier


int main(){

 printf(" Quad Tree ");  
 
 int x,y,n;
 uint64_t img_size; 

 uint8_t *img=stbi_load("test.png",&x,&y,&n,0);

 if(img==NULL){ printf(" Cant Open Image "); }
 
 if(n!=4) { printf(" No Alpha Value \n "); exit(1); }

 QImage qi=QImageInit(img,x,y,n);

 QImage_QuadTreeT1(&qi);
 
 uint8_t pat[4]={0,255,0,255};

 for(uint64_t i=0;i<qi.qt_dbg.next;i++)
 {
  Quad_Border(pat,qi.qt_dbg.quads[i]);
  QImage_BlitQuad(qi,qi.qt_dbg.quads[i]);
 }
  
 

 stbi_write_bmp("ImgOut.bmp",qi.w,qi.h,4,qi.img);
 
 // Quad q=qi.qt_dbg.quads[5];
 // stbi_write_bmp("Quad.bmp",q.w,q.h,4,q.img);
 


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


uint8_t QImage_QuadTreeT1(QImage* qi)
{
 
 // Make As Many Quads as There are Pixels to Later Resize it
 
 qi->qt_dbg.quads=calloc(sizeof(Quad),qi->bytes);
 qi->qp=calloc(sizeof(Quad),qi->bytes);

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
  
  Quad q=qi->qt_dbg.quads[q_num];
  uint64_t limit=q.yb-q.yt;

  //  printf(" %d , %d , %d \n ",q_num,limit,qi->qt_dbg.next);
  
  if(limit==1){ qi->qt_dbg.cnt=qi->qt_dbg.next; break; } 
 
  switch(Quad_IsAlphaOrColor(q)){
   case 0:QImage_SplitQuad(qi,qi->qt_dbg.quads[q_num]);q_num++; printf("1 \n");break; 
   case 1:QImage_SplitQuad(qi,qi->qt_dbg.quads[q_num]);q_num++; printf("2 \n");break; 
   case 2: qi->qp[qi->qp_cnt]=qi->qt_dbg.quads[q_num]; qi->qp_cnt++; qi->qt_dbg.quads[q_num].is_alpha=1; q_num++;  printf("3 \n");break;
   case 3: qi->qp[qi->qp_cnt]=qi->qt_dbg.quads[q_num]; qi->qp_cnt++; qi->qt_dbg.quads[q_num].is_color=1; q_num++;   printf("4 \n");break;
   default: printf(" WHAT DA FUQ HAPPEND  \n "); break ;
  }
 }
  

 printf(" Build Quad Tree With %d Quads",qi->qt_dbg.cnt);
 printf(" Build Quad Tree With Pure %d Quads",qi->qp_cnt);
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
 qi->qt_dbg.quads[qi->qt_dbg.next]=q1;
 qi->qt_dbg.next++;

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
 qi->qt_dbg.quads[qi->qt_dbg.next]=q2;
 qi->qt_dbg.next++;

 
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
 qi->qt_dbg.quads[qi->qt_dbg.next]=q3;
 qi->qt_dbg.next++;
  

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
 qi->qt_dbg.quads[qi->qt_dbg.next]=q4;
 qi->qt_dbg.next++;

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


