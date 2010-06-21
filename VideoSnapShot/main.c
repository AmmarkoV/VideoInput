#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "../VideoInput.h"
#include <linux/videodev2.h>

int main()
{
    printf("Sample program that snaps an image every 10 seconds and outputs it at /home/ammar/public_html/snap0.jpg \n");
    printf("Needs imagemagick for conversions :)\n\n\n");
    printf("Starting Video Input\n");
    if ( InitVideoInputs(1)==1 ) { printf(" .. done \n"); } else
                                 { printf(" .. failed \n"); return 0; }

    char BITRATE=32;
    struct VideoFeedSettings feedsettings={0};
    feedsettings.PixelFormat=V4L2_PIX_FMT_RGB24; BITRATE=24;

    printf("Starting Video Feed\n");
    if ( InitVideoFeed(0,(char *) "/dev/video0",320,240,BITRATE,1,feedsettings)==1  ) { printf(" .. done \n"); } else
                                                                                 { printf(" .. failed \n"); return 0; }

    unsigned int milliseconds=0;
    signed int i;
    while (1)
     {
       GetFrame(0);
       usleep(1000);
       ++milliseconds;

       if ( milliseconds%10000 == 0 )
       {
        RecordOne((char*) "snap");
        i=system((const char *)"convert snap0.ppm /home/ammar/public_html/snap0.jpg");
        milliseconds=0;
       }
     }



    printf("Closing Video Feeds\n");
    CloseVideoInputs();
    return 0;
}
