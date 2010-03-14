#include "image_storage.h"
#include <stdio.h>
#include <stdlib.h>


int ReadDoubleRAW(char * filename,struct Image * left_pic,struct Image * right_pic)
{
   unsigned int temp=0,size_of_image=0;
   unsigned int mem_read1,mem_read2;
   unsigned int size_x,size_y,depth;
   FILE *file;
   long lSize=0;
   file = fopen(filename,"rb");

    if (file!=0)
	{
     fseek (file , 0 , SEEK_END);
     lSize = ftell (file);
     rewind (file);

     fscanf(file,"%u\n",&size_x);
     fscanf(file,"%u\n",&size_y);
     fscanf(file,"%u\n",&depth);

     if ( ( left_pic->depth != depth ) || ( left_pic->size_x!=size_x ) || ( left_pic->size_y!=size_y ) ) { fprintf(stderr,"!!!!ERROR WRONG SIZE OF MEMORY INITIALIZED :S !!!!\n"); }
     fprintf(stderr,"Reading file %u x %u @ %u \n",size_x,size_y,depth);

     size_of_image =  (unsigned int) left_pic->size_x * (unsigned int)  left_pic->size_y * (unsigned int) left_pic->depth;
     mem_read1=fread ( left_pic->pixels , 1 , size_of_image , file );
     fprintf(stderr,"Read first frame %u \n" , mem_read1);

     fscanf(file,"%u",&temp);
     if ( temp!=0 ) { fprintf(stderr,"Intermidiate byte not null!\n"); }

     mem_read2=fread ( right_pic->pixels , 1 , size_of_image , file );
     fprintf(stderr,"Read second frame %u \n" , mem_read2);

     fscanf(file,"%u",&temp);

     fclose(file);
     return 1;
	}

  return 0;
}


int WriteDoubleRAW(char * filename,struct Image * left_pic,struct Image * right_pic)
{
   unsigned int temp=0,size_of_image=0;
   unsigned int mem_write1,mem_write2;
   FILE *file;
   file = fopen(filename,"wb");

    if (file!=0)
	{
     fprintf(stderr,"Writing file %u x %u @ %u \n",left_pic->size_x,left_pic->size_y,left_pic->depth);

     fprintf(file,"%u\n",left_pic->size_x);
     fprintf(file,"%u\n",left_pic->size_y);
     fprintf(file,"%u\n",left_pic->depth);

     size_of_image =  (unsigned int) left_pic->size_x * (unsigned int)  left_pic->size_y * (unsigned int) left_pic->depth;

     mem_write1=fwrite ( left_pic->pixels , 1 , size_of_image , file );
     fprintf(stderr,"Write first frame %u \n" , mem_write1);

     fprintf(file,"%u",temp);

     mem_write2=fwrite ( right_pic->pixels , 1 , size_of_image , file );
     fprintf(stderr,"Writing second frame %u \n" , mem_write2);

     fprintf(file,"%u",temp);

     fflush ( file );
     fclose(file);


     fprintf(stderr,"Returning\n");
     return 1;
	}
 return 0;
}

int ClearImage(struct Image * pic )
{
    return 0;
}
