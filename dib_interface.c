#include "dib_interface.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct bitmapFileHeader
{
   unsigned short int bfType;
   unsigned int bfSize;
   unsigned short int bfReserved1, bfReserved2;
   unsigned int bfOffBits;
} BITMAPFILEHEADER;

typedef struct bitmapInfoHeader
{
   unsigned int biSize;
   int biWidth, biHeight;
   unsigned short int biPlanes;
   unsigned short int biBitCount;
   unsigned int biCompression;
   unsigned int biSizeImage;
   int biXPelsPerMeter, biYPelsPerMeter;
   unsigned int biClrUsed;
   unsigned int biClrImportant;
} BITMAPINFOHEADER;

typedef struct colourIndex
{
   unsigned char r,g,b,junk;
} COLOURINDEX;


unsigned char *LoadBitmapFile(char *argv[], BITMAPINFOHEADER *bitmapInfoHeader)
{
FILE *file1;
BITMAPFILEHEADER bitmapFileHeader;
unsigned char *bitmapImage;
int imageIdx=0;
unsigned char tempRGB;


file1 = fopen(argv[1], "rb");
if (file1 == NULL)
return NULL;


fread(&bitmapFileHeader, sizeof(BITMAPFILEHEADER), 1, file1);


if (bitmapFileHeader.bfType !=0x4D42)
{
fclose(file1);
return NULL;
}

fread(bitmapInfoHeader, sizeof(BITMAPINFOHEADER),1,file1);


fseek(file1, bitmapFileHeader.bfOffBits, SEEK_SET);


bitmapImage = (unsigned char*)malloc(bitmapInfoHeader->biSizeImage);


if (!bitmapImage)
{
free(bitmapImage);
fclose(file1);
return NULL;
}


fread(bitmapImage,bitmapInfoHeader->biSizeImage,1,file1);


if (bitmapImage == NULL)
{
fclose(file1);
return NULL;
}


fclose(file1);
return bitmapImage;


}
