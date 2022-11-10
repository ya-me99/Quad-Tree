
typedef struct
{
 Image img;
 uint32_t* pairs;
}Shape;

uint8_t Shape_FindPairs(Shape* s);
void Shape_Fill(Shape s);

uint8_t  Casteljau(float* pts , uint64_t count,
                   uint64_t t , float scale , Image* image);

uint8_t Bernstein(float* pts , uint64_t count,
                    uint64_t t , float scale, Image* image);


int main(){

 float test[24]={0.25,0.25,0.50,0.0,0.75,0.25,  
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
  
 return 1;
}

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
