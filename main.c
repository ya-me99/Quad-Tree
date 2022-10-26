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
 uint8_t is_alpha;
 uint8_t* img;
}Quad;

typedef struct 
{
 Quad* quads;
 uint8_t lock;
 uint64_t next;
}QuadTree;

typedef struct
{
 uint8_t* img;
 uint64_t w,h;
 uint8_t ch;
 uint8_t*  pix;   // Direct Map 
 QuadTree qt1;
}QImage;

uint8_t QImage_PixMap(QImage* qi);
uint8_t QImage_QuadTreeT1(QImage* qi);
uint8_t Quad_Image(QImage qi,Quad *q);
uint8_t QImage_SplitQuad(QImage* qi, Quad q);

int main(){

 printf(" Quad Tree ");  
 
 int x,y,n;
 uint64_t img_size; 

 uint8_t *img=stbi_load("test.png",&x,&y,&n,0);

 if(img==NULL){ printf(" Cant Open Image "); }
 
 if(n!=4) { printf(" No Alpha Value \n "); exit(1); }

 img_size=x*y*n;
 
 uint8_t* img_out=calloc(1,img_size);

 for(uint64_t i=0;i<img_size;i+=4){
  
  uint8_t r,g,b,a;
  
  r=img[i];  
  g=img[i+1];  
  b=img[i+2];  
  a=img[i+3];
  
  img_out[i]=(r+b+g)/3;
  img_out[i+1]=(r+b+g)/3;
  img_out[i+2]=(r+b+g)/3;
  img_out[i+3]=0;
 }
 
 stbi_write_bmp("ImgOut.bmp",x,y,n,img_out);
 
 return 0;
}

QImage QImageInit(uint8_t* img, uint64_t x, uint64_t y , uint8_t ch)
{
 
 QImage qi={ };
 QuadTree qt1={ };

 qi.img=img;
 qi.w=x;
 qi.h=y;
 qi.ch=ch;
 qi.qt1=qt1;
 return qi;
}

// Todo: Use Bits Not Bytes 

uint8_t QImage_PixMap(QImage* qi)
{
 
 uint64_t QuadCount=qi->w*qi->h;            
 
 qi->pix=calloc(1,QuadCount);

 for(uint64_t i=0;i<QuadCount;i+=4){
  
  if(!qi->img[i+3]==0) { qi->pix[i]=1; };
 }
 
}


uint8_t QImage_QuadTreeT1(QImage* qi)
{
 
 // Make As Many Quads as There are Pixels to Later Resize it
 
 Quad mq={ 0,qi->w,0,qi->h };

 QImage_SplitQuad(qi,mq);
 
 // if( Quad_isAlpha( mq ) )

}


uint8_t QImage_SplitQuad(QImage* qi, Quad q)
{
  
 Quad q1,q2,q3,q4 = { }; 
 
 uint64_t q_size=(q.xb-q.xt)*(q.yb-q.yt)*qi->ch;

 // Top Left Split 
 q1.xt=q.xt;
 q1.xb=q.xt+(q.xb-q.xt)/2;
 q1.yt=q.yt;
 q1.yb=q.yt+(q.yb-q.yt)/2;
 
 q1.img=calloc(1,q_size);
 
 // Top Right Split 
 q2.xt=q.xt+(q.xb-q.xt)/2;
 q2.xb=q.xb;
 q2.yt=q.yt;
 q2.yb=q.yt+(q.yb-q.yt)/2;

 // Top Left Split 
 q3.xt=q.xt;
 q3.xb=q.xt+(q.xb-q.xt)/2;
 q3.yt=q.yt+(q.yb-q.yt)/2;
 q3.yb=q.yb;

 // Top Right Split 
 q4.xt=q.xt+(q.xb-q.xt)/2;
 q4.xb=q.xb;
 q4.yt=q.yt+(q.yb-q.yt)/2;
 q4.yb=q.yb;

}


uint8_t Quad_Image(QImage qi,Quad *q)
{
 
 uint64_t h_size=q->yb-q->yt;
 uint64_t l_size=q->xb-q->xt;


 for(uint64_t i=0;i<h_size*l_size;i+=l_size)
 {
  memcpy(&(qi.img[q->xt+i]),&(q->img[i]),l_size);
 }

}


/*

uint8_t Quad_IsAlpha(Quad q )
{

 uint64_t q_size=q.yb-q.yt;
 uint64_t l_size=q.xb-q.xt;

 for(uint64_t i=0;i<q_size;i++){
  uint64_t offset=q.xt+i*l_size;
  
  if(!QuadTree_LineIsAlpha(x_top,x_bot+offset,img)){
      printf("Isnt \n"); return 0; 
     }
 }
 
 printf(" Is \n ");
 return 1;
}
 


uint8_t QuadTree_LineIsAlpha(uint64_t start,uint64_t end,uint8_t* img){

 uint64_t l_size=end-start;
 for(uint64_t i=0;i<l_size+=4){
  if(img[start+i+3]!=0){ return 0;}
 } 
}


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
