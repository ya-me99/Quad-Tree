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
 uint64_t xt,xb,yt,yb;
 uint64_t w,h;
 uint64_t bytes_per_line;
 uint64_t bytes;
 uint8_t ch;
 uint8_t* img;
 uint8_t is_alpha;
}Quad;

typedef struct 
{
 Quad* quads;
 uint8_t lock;
 uint64_t next;
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


int main(){

 printf(" Quad Tree ");  
 
 int x,y,n;
 uint64_t img_size; 

 uint8_t *img=stbi_load("test.png",&x,&y,&n,0);

 if(img==NULL){ printf(" Cant Open Image "); }
 
 if(n!=4) { printf(" No Alpha Value \n "); exit(1); }

 img_size=x*y*n;
 
 //uint8_t* img_out=calloc(1,img_size);

 QImage qi=QImageInit(img,x,y,n);
 
 QImage_QuadTreeT1(&qi);
  
 for(uint8_t i=0;i<qi.qt1.next;i++)
 {
  QImage_BlitQuad(qi,qi.qt1.quads[i*4]);
 }
 
 stbi_write_bmp("ImgOut.bmp",qi.w,qi.h,qi.ch,qi.img);
 
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
 
 qi->qt1.quads=calloc(1,qi->bytes);

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

 while(1)
 {
  uint64_t limit=qi->qt1.quads[q_num].yb-qi->qt1.quads[q_num].yt;

  if(Quad_IsAlpha(qi->qt1.quads[q_num]) || limit<=1 ){ break ;}
  
  uint8_t pat[4]={q_num*20,0,0,255};
  Quad_Fill(qi->qt1.quads[q_num],pat);
  QImage_SplitQuad(qi,qi->qt1.quads[q_num]);
  q_num+=4;
 }

}



uint8_t QImage_SplitQuad(QImage* qi, Quad q)
{
  
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

 printf(" %d , %d \n ",q1.w,q.w);
 Quad_Image(*qi,&q1);
 
 qi->qt1.quads[qi->qt1.next]=q1;
 qi->qt1.next++;

 // Top Right Split 
 q2.xt=q.xt+(q.w)/2;
 q2.xb=q.xb;
 q2.yt=q.yt;
 q2.yb=q.yt+(q.h)/2;
 q2.ch=qi->ch;  

 q2.w=q2.xb-q2.xt;
 q2.h=q2.yb-q2.yb; 
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
 q3.h=q3.yb-q3.yb; 
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
 q4.h=q4.yb-q4.yb; 
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
  uint64_t off2=(q->yt+i)*qi.bytes_per_line+q->xt;

  memcpy(&(q->img[off1]),&(qi.img[off2]),q->bytes_per_line);
 }

}


uint8_t Quad_IsAlpha(Quad q){

 for(uint64_t i=0;i<q.h*q.w;i+=4){
  
  if(!q.img[i+3]==0){ printf(" Isnt \n "); return 0 ; }

 }
 
 printf(" Is \n ");
 return 1;
}



/*

uint8_t QuadTree_QuadIsAlpha(uint64_t x_top,uint64_t x_end,
                             uint64_t y_top,uint64_t y_end,uint8_t* img){
  
 uint64_t q_size=y_bot-y_top;
 uint64_t l_size=x_bot-x_top;

 for(uint64_t i=0;i<q_size;i++){
  uint64_t offset=x_top+i*l_size;
  
  if(!QuadTree_LineIsAlpha(x_top,x_bot+offset,img)){
      printf("Isnt \n"); return 0; 
     }
 }
 
 printf(" Is \n ");
 return 1;
}
*/
