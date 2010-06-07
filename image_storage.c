/***************************************************************************
* Copyright (C) 2010 by Ammar Qammaz *
* ammarkov@gmail.com *
* *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 2 of the License, or *
* (at your option) any later version. *
* *
* This program is distributed in the hope that it will be useful, *
* but WITHOUT ANY WARRANTY; without even the implied warranty of *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the *
* GNU General Public License for more details. *
* *
* You should have received a copy of the GNU General Public License *
* along with this program; if not, write to the *
* Free Software Foundation, Inc., *
* 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. *
***************************************************************************/

#include "image_storage.h"
#include <stdio.h>
#include <stdlib.h>



int ReadRAW(char * filename,struct Image * pic)
{
   unsigned int temp=0,size_of_image=0;
   unsigned int mem_read1;
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

     if ( ( pic->depth != depth ) || ( pic->size_x!=size_x ) || ( pic->size_y!=size_y ) ) { fprintf(stderr,"!!!!ERROR WRONG SIZE OF MEMORY INITIALIZED :S !!!!\n"); }
     fprintf(stderr,"Reading file %u x %u @ %u \n",size_x,size_y,depth);

     size_of_image =  (unsigned int) pic->size_x * (unsigned int)  pic->size_y * (unsigned int) pic->depth;
     mem_read1=fread ( pic->pixels , 1 , size_of_image , file );
     fprintf(stderr,"Read first frame %u \n" , mem_read1);


     fclose(file);
     return 1;
	}
 return 0;
}



int WriteRAW(char * filename,struct Image * pic)
{
 unsigned int temp=0,size_of_image=0;
   unsigned int mem_write1;
   FILE *file;
   file = fopen(filename,"wb");

    if (file!=0)
	{
     fprintf(stderr,"Writing file %u x %u @ %u \n", pic->size_x, pic->size_y, pic->depth);

     fprintf(file,"%u\n", pic->size_x);
     fprintf(file,"%u\n", pic->size_y);
     fprintf(file,"%u\n", pic->depth);

     size_of_image =  (unsigned int)  pic->size_x * (unsigned int)   pic->size_y * (unsigned int)  pic->depth;

     mem_write1=fwrite (  pic->pixels , 1 , size_of_image , file );

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


int ConvertImageFormats(char * filenamein,char * filenameout)
{ //Needs imagemagick package :)
 char execstr[256]={0};
 sprintf(execstr,"convert %s %s",filenamein,filenameout);
 system(execstr);
}

int ConvertSnapshotsToVideo(int framerate,int bitrate,char * filenameout)
{
 // ffmpeg -r 10 -b 1800 -i %03d.jpg test1800.mp4
 char execstr[256]={0};
 sprintf(execstr,"ffmpeg -r %u -b %u -i \%05d.jpg %s.mp4",framerate,bitrate,filenameout);
 system(execstr);
}


