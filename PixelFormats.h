#include <stdlib.h>
#include <stdio.h>
#include <linux/videodev2.h>


int VideoFormatNeedsDecoding(int videoformat,int bitdepth);

void PrintOutPixelFormat(int pix_format);
void PrintOutCaptureMode(int cap_mode);
void PrintOutFieldType(int field_type);

//unsigned char *YUV420_to_RGB24(unsigned char *b1,int width,int height,unsigned char *b2=NULL);
