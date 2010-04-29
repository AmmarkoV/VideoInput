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

#include <stdio.h>
#include <stdlib.h>
#include "../VideoInput.h"

int main()
{
    printf("Testing VideoInput v%s !\n",VideoInput_Version());
    printf("This should compile ok provided that you link libVideoInput.a and pthreads\n");
    printf("For now the whole library is linux only , I have a windows version of the library\n");
    printf("I will have to merge them when I have the time..\n");
    printf("-----------------------------------------------------------\n");


    printf("I Will now attempt to start Video Devices with a total maximum number of 3 video devices! :) \n");
    InitVideoInputs(3);

    printf("I Will now attempt to start sampling on VideoDevice slot 0  :) \n");
    printf("The function called will be : InitVideoFeed(0,\"/dev/video0\",320,240,1);\n");
    printf("0 is the first device , /dev/video0 the linux location of the device , 320x240 the size of the picture and 1 means enable snapshots!\n");
    InitVideoFeed(0,"/dev/video0",320,240,1);

    printf("Waiting for loop to begin receiving video ");
    int waittime=0;
    while ( !FeedReceiveLoopAlive(0) )
      { printf("."); ++waittime; }
    printf(" ok! \n");


    printf("The Video feed is now beeing read from a secondary thread so each time we need a new snap we \"GetFrame\"\n");
    printf("unsigned char * pixels  is the pointer to the image data , this minimizes copying through memory\n");
    unsigned char * pixels = 0;


    printf("I Will now attempt to catch the frame \n");
    pixels = (unsigned char *) GetFrame(0);

    if (pixels == 0 ) { printf("Something was not right and we got back a zero frame , test failed\n");
                        CloseVideoInputs();
                        return 1;}


    printf("I Will now extract the RGB value from pixel 0,0 and pixel 10,40 and 11,40 \n");
    int x=0,y=0;

    unsigned char *px=0;
    unsigned char *r=0;
    unsigned char *g=0;
    unsigned char *b=0;

    px=pixels+ (y*320*3 + x*3);
    r=px++; g=px++; b=px;
    printf("The RGB value of pixel %u,%u is %u %u %u \n",x,y,*r,*g,*b);

    x=10; y=40;
    px=pixels+ (y*320*3 + x*3);
    r=px++; g=px++; b=px++;
    printf("The RGB value of pixel %u,%u is %u %u %u \n",x,y,*r,*g,*b);

    r=px++; g=px++; b=px;
    printf("The RGB value of pixel %u,%u is %u %u %u \n",x+1,y,*r,*g,*b);


    printf ("I Will now try to write what the camera is seeing in a file called now.raw  ... ");
    RecordOne((char*) "raw");
    sleep(1);
    printf("Done");

    printf ("I Will now try to emulate camera input using a file called snapped.raw  ... ");
    Play((char*) "snapped");
    sleep(1);
    printf("Done");

    printf ("I Will now try to stop emulation and resume the live feed  ... ");
    Stop();
    sleep(1);
    printf("Done");


    printf("All tests are complete , closing video inputs!\n");
    CloseVideoInputs();
    printf("Done..!\n");
    return 0;
}
