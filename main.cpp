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

#include "VideoInput.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "V4L2.h"
#include "PixelFormats.h"
#include "PixelFormatConversions.h"
#include "image_storage.h"
#include <unistd.h>

#define LIVE_ON 0
#define RECORDING_ON 1
#define RECORDING_ONE_ON 2
#define PLAYBACK_ON 3
#define PLAYBACK_ON_LOADED 4
#define WORKING 5
#define NO_VIDEO_AVAILIABLE 6

char * VIDEOINPT_VERSION=(char *) "0.266 UNSTABLE";
int increase_priority=0;

struct Video
{
  /* DEVICE NAME */
  char * videoinp;
  unsigned int height;
  unsigned int width;

  /* VIDEO 4 LINUX DATA */
  struct v4l2_format fmt;
  void *frame;
  unsigned int size_of_frame;
  V4L2 *v4l2_intf;

  /* DATA NEEDED FOR DECODERS TO WORK */
  unsigned int input_pixel_format;
  unsigned int input_pixel_format_bitdepth;
  char * decoded_pixels;
  int frame_decoded;

  /*VIDEO SIMULATION DATA*/
  struct Image rec_video;
  int video_simulation;

  /* THREADING DATA */
  int thread_alive_flag;
  int snap_paused; /* If set to 1 Will continue to snap frames but not save the reference ( that way video loop wont die out ) */
  int snap_lock; /* If set to 1 Will not snap frames at all ( video loop will die out after a while ) */
  int stop_snap_loop;
  pthread_t loop_thread;
};

struct ThreadPassParam
{
    int feednum;
};

int total_cameras=0;
struct Video * camera_feeds=0;
char video_simulation_path[256]={0};
io_method io=IO_METHOD_MMAP; //IO_METHOD_MMAP; // IO_METHOD_READ; //IO_METHOD_USERPTR;

void * SnapLoop(void *ptr );



char * VideoInput_Version()
{
  return VIDEOINPT_VERSION;
}


char FileExists(char * filename)
{
FILE *fp = fopen(filename,"r");
 if( fp ) { /* exists */
            fclose(fp);
            return 1;
          }
          else
          { /* doesnt exist */ }
 return 0;
}


int VideoInputsOk()
{
  if ( total_cameras == 0 ) { fprintf(stderr,"Error TotalCameras == zero\n"); return 0; }
  if ( camera_feeds == 0 ) { fprintf(stderr,"Error CameraFeeds == zero\n");  return 0; }

 return 1;
}

int InitVideoInputs(int numofinputs)
{
    if (total_cameras>0) { fprintf(stderr,"Error , Video Inputs already active ?\n total_cameras=%u\n",total_cameras); return 0;}

    //First allocate memory for V4L2 Structures  , etc
    camera_feeds = (struct Video * ) malloc ( sizeof( struct Video ) * (numofinputs+1) );
    if (camera_feeds==0) { fprintf(stderr,"Error , cannot allocate memory for %u video inputs \n",total_cameras); return 0;}

    int i;
    for ( i=0; i<total_cameras; i++ )
      {  /*We mark each camera as dead , to preserve a clean state*/
          camera_feeds[i].thread_alive_flag=0;
          camera_feeds[i].rec_video.pixels=0;
          camera_feeds[i].frame=0;
          camera_feeds[i].v4l2_intf=0;
      }

    //Lets Refresh USB devices list :)
    int ret=system((const char * ) "lsusb");
    if ( ret == 0 ) { printf("Syscall USB list success\n"); }

    printf("\nAvailiable Video Devices : \n");
    ret=system((const char * ) "ls /dev | grep video");
    if ( ret == 0 ) { printf("Success receiving video device list \n"); }

    //We want higher priority now..! :)
    if ( increase_priority == 1 )
    {
     if ( nice(-4) == -1 ) { fprintf(stderr,"Error increasing priority on main video capture loop\n");} else
                           { fprintf(stderr,"Increased priority \n"); }
    }

    total_cameras=numofinputs;

    return 1 ;
}

int CloseVideoInputs()
{
    if (total_cameras==0) { fprintf(stderr,"Error , Video Inputs already deactivated ?\n"); return 0;}
    if (camera_feeds==0) { fprintf(stderr,"Error , Video Inputs already deactivated ?\n"); return 0;}

    int i=0;
    for ( i=0; i<total_cameras; i++ )
     {
       if (camera_feeds[i].thread_alive_flag)
       {
        fprintf(stderr,"Video %u Stopping\n",i);
        camera_feeds[i].stop_snap_loop=1;
        pthread_join( camera_feeds[i].loop_thread, NULL);
        usleep(30);
        camera_feeds[i].v4l2_intf->stopCapture();
        usleep(30);
        camera_feeds[i].v4l2_intf->freeBuffers();
        usleep(30);

        if ( camera_feeds[i].decoded_pixels !=0 ) free( camera_feeds[i].decoded_pixels );
        if ( camera_feeds[i].rec_video.pixels !=0 ) free( camera_feeds[i].rec_video.pixels );
        if ( camera_feeds[i].v4l2_intf != 0 ) { delete camera_feeds[i].v4l2_intf; }
       } else
       {
        fprintf(stderr,"Video Feed %u seems to be already dead , ensuring no memory leaks!\n",i);
        camera_feeds[i].stop_snap_loop=1;
        if ( camera_feeds[i].rec_video.pixels !=0 ) free( camera_feeds[i].rec_video.pixels );
        if ( camera_feeds[i].v4l2_intf != 0 ) { delete camera_feeds[i].v4l2_intf; }
       }
     }



    fprintf(stderr,"Deallocation of Video Structures\n");

    free(camera_feeds);

    fprintf(stderr,"Video Input successfully deallocated\n");

    return 1 ;
}


int InitVideoFeed(int inpt,char * viddev,int width,int height,int bitdepth,char snapshots_on,struct VideoFeedSettings videosettings)
{
   printf("Initializing Video Feed %u ( %s ) @ %u/%u \n",inpt,viddev,width,height);
   if (!VideoInputsOk()) return 0;
   if ( (!FileExists(viddev)) ) { fprintf(stderr,"Super Quick linux check for the webcam (%s) returned false.. please connect V4L2 compatible camera!\n",viddev); return 0; }

   camera_feeds[inpt].videoinp = viddev; //px (char *) "/dev/video0";
   camera_feeds[inpt].width = width;
   camera_feeds[inpt].height = height;
   camera_feeds[inpt].size_of_frame=width*height*(bitdepth/8);
   camera_feeds[inpt].video_simulation=LIVE_ON;
   camera_feeds[inpt].thread_alive_flag=0;
   camera_feeds[inpt].snap_paused=0;
   camera_feeds[inpt].snap_lock=0;

   camera_feeds[inpt].frame_decoded=0;
   camera_feeds[inpt].decoded_pixels=0;

   CLEAR (camera_feeds[inpt].fmt);
   camera_feeds[inpt].fmt.fmt.pix.width       = width;
   camera_feeds[inpt].fmt.fmt.pix.height      = height;


   /* IF videosettings is null set default capture mode ( VIDEO CAPTURE , YUYV mode , INTERLACED )  */
      if ( videosettings.EncodingType==0 ) { camera_feeds[inpt].fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; } else
                                           { camera_feeds[inpt].fmt.type = (v4l2_buf_type) videosettings.EncodingType; }

      if ( videosettings.PixelFormat==0 ) { camera_feeds[inpt].fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV; } else
                                          { camera_feeds[inpt].fmt.fmt.pix.pixelformat = videosettings.PixelFormat; }

      if ( videosettings.FieldType==0 ) { camera_feeds[inpt].fmt.fmt.pix.field = V4L2_FIELD_INTERLACED; } else
                                        { camera_feeds[inpt].fmt.fmt.pix.field = (v4l2_field) videosettings.FieldType; }

      camera_feeds[inpt].input_pixel_format=camera_feeds[inpt].fmt.fmt.pix.pixelformat;
      camera_feeds[inpt].input_pixel_format_bitdepth=bitdepth;


      PrintOutCaptureMode(camera_feeds[inpt].fmt.type);
      PrintOutPixelFormat(camera_feeds[inpt].fmt.fmt.pix.pixelformat);
      PrintOutFieldType(camera_feeds[inpt].fmt.fmt.pix.field);

       camera_feeds[inpt].decoded_pixels=0;
       if (VideoFormatNeedsDecoding(camera_feeds[inpt].input_pixel_format,camera_feeds[inpt].input_pixel_format_bitdepth))
       {
          //DECODE TO RGB 24
          camera_feeds[inpt].decoded_pixels = (char * ) malloc( width*height*3 + 1);
          memset(camera_feeds[inpt].decoded_pixels, '\0',width*height*3);
       }



      fprintf(stderr,(char *)"Starting camera , if it segfaults consider running \nLD_PRELOAD=/usr/lib/libv4l/v4l2convert.so  executable_name\n");
      fprintf(stderr,(char *)"Your webcam might not support V4L2!\n");
      fprintf(stderr,(char *)"------------------------------------------------\n");
      camera_feeds[inpt].v4l2_intf = new V4L2(camera_feeds[inpt].videoinp, io);
       if ( camera_feeds[inpt].v4l2_intf->set(camera_feeds[inpt].fmt) == 0 ) { fprintf(stderr,"Device does not support settings:\n"); return 0; }
         else
       {
           fprintf(stderr,"No errors , starting camera %u / locking memory..!",inpt);
           camera_feeds[inpt].v4l2_intf->initBuffers();
           camera_feeds[inpt].v4l2_intf->startCapture();

           camera_feeds[inpt].frame = 0;
       }

   printf("Enabling Snapshots!\n");
   camera_feeds[inpt].rec_video.pixels = 0; // Xreiazontai etsi wste an den theloume snapshots na min crasharei to sympan
   camera_feeds[inpt].rec_video.size_x=width;
   camera_feeds[inpt].rec_video.size_y=height;
   camera_feeds[inpt].rec_video.depth=3;
   if ( snapshots_on == 1 )
    {
        camera_feeds[inpt].rec_video.pixels = (char * ) malloc( camera_feeds[inpt].size_of_frame + 1);
    }
    // INIT MEMORY FOR SNAPSHOTS !


    // STARTING VIDEO RECEIVE THREAD!
    camera_feeds[inpt].stop_snap_loop=0;
    camera_feeds[inpt].loop_thread=0;

    struct ThreadPassParam param={0};
    param.feednum=inpt;
    pthread_create( &camera_feeds[inpt].loop_thread, NULL,  SnapLoop ,(void*) &param);

    int timeneeded=0;
    while (camera_feeds[inpt].thread_alive_flag==0) { usleep(20); ++timeneeded; printf("."); }

    printf("Giving some time for the receive threads to wake up!\n");
    sleep(1);
    printf("InitVideoFeed %u is ok!\n",inpt);


  return 1;
}



int PauseFeed(int feednum)
{
 if (!VideoInputsOk()) return 0;
 //camera_feeds[feednum].snap_lock=1;
 camera_feeds[feednum].snap_paused=1;
 return 1;
}

int UnpauseFeed(int feednum)
{
 if (!VideoInputsOk()) return 0;
 //camera_feeds[feednum].snap_lock=0;
 camera_feeds[feednum].snap_paused=0;
 return 1;
}


int DecodePixels(int webcam_id)
{
if ( camera_feeds[webcam_id].frame_decoded==0)
                                             { //THIS FRAME HASN`T BEEN DECODED YET!
                                               int i=Convert2RGB24( (char*)camera_feeds[webcam_id].frame,
                                                                    (char*)camera_feeds[webcam_id].decoded_pixels,
                                                                    camera_feeds[webcam_id].width,
                                                                    camera_feeds[webcam_id].height,
                                                                    camera_feeds[webcam_id].input_pixel_format,
                                                                    camera_feeds[webcam_id].input_pixel_format_bitdepth );

                                               if ( i == 0 ) { /* UNABLE TO PERFORM CONVERSION */
                                                                return 0; } else

                                                              { /* SUCCESSFUL CONVERSION */
                                                                  camera_feeds[webcam_id].frame_decoded=1;
                                                              }
                                             }
 return 1;
}

unsigned char * ReturnDecodedLiveFrame(int webcam_id)
{
   if (VideoFormatNeedsDecoding(camera_feeds[webcam_id].input_pixel_format,camera_feeds[webcam_id].input_pixel_format_bitdepth)==1)
                                          {
                                            //VIDEO COMES IN A FORMAT THAT NEEDS DECODING TO RGB 24
                                            printf("Passing by Decoding Pixels to RGB24\n");
                                            if ( DecodePixels(webcam_id)==0 ) return 0;
                                            return (unsigned char *) camera_feeds[webcam_id].decoded_pixels;
                                          } else
                                          {
                                            printf("Passing RAW Frame\n");
                                            return (unsigned char *) camera_feeds[webcam_id].frame;
                                          }
   return 0;
}

unsigned char * GetFrame(int webcam_id)
{
  if (!VideoInputsOk()) return 0;
  int handled=0;

  switch(camera_feeds[webcam_id].video_simulation)
  {
    case  LIVE_ON :
     {
      handled=1;
      if (total_cameras>webcam_id) {
                                    return ReturnDecodedLiveFrame(webcam_id);
                                   }
                                     else
                                   { return 0; }
     }
    break;

    case  PLAYBACK_ON_LOADED :
     {
      handled=1;
      if (total_cameras>webcam_id) {  return (unsigned char *) camera_feeds[webcam_id].rec_video.pixels; } else
                                   { return 0; }
     }
    break;

    case  PLAYBACK_ON :
     {
      handled=1;
      fprintf(stderr,"Reading Snapshot\n");

      char store_path[256]={0};
      char last_part[6]="0.ppm";
      last_part[0]='0'+webcam_id;

      strcpy(store_path,video_simulation_path);
      strcat(store_path,last_part);
      ReadPPM(store_path,&camera_feeds[webcam_id].rec_video);
      camera_feeds[webcam_id].video_simulation = PLAYBACK_ON_LOADED;
      fprintf(stderr,"Reading Snapshot ( %s ) \n",store_path);
      return (unsigned char *) camera_feeds[webcam_id].rec_video.pixels;
     }
    break;

    case NO_VIDEO_AVAILIABLE :
     {
     /* VIDEO DEVICE COULD BE DEAD , DO NOTHING!*/
      handled=1;
     }
    break;

     default :
     {
      /* THE FRAME MODE IS SET ON AN UNKNOWN MODE*/
      handled=0;
     }
  }
  return 0;
}



void RecordInLoop(int feed_num)
{
    fprintf(stderr,"called record in loop\n");
    unsigned int mode_started = camera_feeds[feed_num].video_simulation;
    camera_feeds[feed_num].video_simulation = WORKING;
    camera_feeds[feed_num].snap_lock=1;
    fprintf(stderr,"trying to memcpy\n");
    fflush(0);
    usleep(10);

    memcpy(camera_feeds[feed_num].rec_video.pixels,ReturnDecodedLiveFrame(feed_num),camera_feeds[feed_num].size_of_frame);
    fprintf(stderr,"survived memcpy\n");
    fflush(0);
    camera_feeds[feed_num].snap_lock=0;


    char store_path[256]={0};
    char last_part[6]="0.ppm";
    last_part[0]='0'+feed_num;

    strcpy(store_path,video_simulation_path);
    strcat(store_path,last_part);
    fprintf(stderr,"WritePPM\n");
    WritePPM(store_path,&camera_feeds[feed_num].rec_video);
    if ( mode_started == RECORDING_ONE_ON) { camera_feeds[feed_num].video_simulation = LIVE_ON; }

    fprintf(stderr,"Writing Snapshot ( %s ) \n",store_path);

    return;
}

int FeedReceiveLoopAlive(int feed_num)
{
 if (!VideoInputsOk()) return 0;
 if ( feed_num >= total_cameras ) return 0;
 return camera_feeds[feed_num].thread_alive_flag;
}


void * SnapLoop( void * ptr)
{
    printf("Starting SnapLoop!\n");
    /*Adapt ptr to feednum to pass value to feednum */
    struct ThreadPassParam *param;
    param = (struct ThreadPassParam *) ptr;
    int feed_num=param->feednum;

    if ( feed_num >= total_cameras )
     {
       printf("Error passing value of feed to the new thread\n");
       printf("The new thread has no idea of what feed it is supposed to grab frames from so it will fail now!\n");
       return 0;
     }

   //We want higher priority now..! :)
    if ( increase_priority == 1 )
    {
     if ( nice(-4) == -1 ) { fprintf(stderr,"Error increasing priority on main video capture loop\n");} else
                           { fprintf(stderr,"Increased priority \n"); }

    }

   printf("Video capture thread #%d is alive \n",feed_num);
   camera_feeds[feed_num].thread_alive_flag=1;

   while ( camera_feeds[feed_num].stop_snap_loop == 0 )
    {
       usleep(5); /* 20ms sleep time per sample , its a good value for 2 cameras*/

       if ( camera_feeds[feed_num].snap_lock == 0 )
       { // WE DONT NEED THE SNAPSHOT TO BE LOCKED!
          if ( camera_feeds[feed_num].snap_paused == 1 )
           { camera_feeds[feed_num].v4l2_intf->getFrame(); /*Get frame only to keep V4L2 running ? */ } else
           { camera_feeds[feed_num].frame=camera_feeds[feed_num].v4l2_intf->getFrame(); }

          camera_feeds[feed_num].frame_decoded=0; //<- This signals that we have a new frame that MAY need to be decoded to RGB24
       } else
       {
         /* FEED LOCKED */
       }

      /*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
        SNAPSHOT RECORDING*/
       if ( (camera_feeds[feed_num].video_simulation == RECORDING_ON ) || (camera_feeds[feed_num].video_simulation == RECORDING_ONE_ON) )
       {
            if ( camera_feeds[feed_num].frame == 0 ) { fprintf(stderr,"Do not want Null frames recorded :P \n"); }
              else
                RecordInLoop(feed_num);
       }
      /*SNAPSHOT RECORDING
        >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
    }
   printf("Video capture thread #%u is closing..! \n",feed_num);

   /* !NOTICE! THE FOLLOWING LINE COUD BE DISABLED TO ENABLE REPLAYING EVEN IF VIDEO FAILS*/
   camera_feeds[feed_num].video_simulation = NO_VIDEO_AVAILIABLE; // NO VIDEO MEANS NO_VIDEO_AVAILIABLE


   camera_feeds[feed_num].thread_alive_flag=0;
   return ( void * ) 0 ;
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// SNAPSHOT RECORDING
void Play(char * filename)
{
    if (!VideoInputsOk()) return;
    if ( strlen( filename ) > 250 ) return;

    //Prepare
    strcpy(video_simulation_path,filename);


    int i=0;
    for (i=0; i<total_cameras; i++)  camera_feeds[i].video_simulation = PLAYBACK_ON;

}

void Record(char * filename)
{
    if (!VideoInputsOk()) return;
    if ( strlen( filename ) > 250 ) return;

    int i=0;
    for (i=0; i<total_cameras; i++)  camera_feeds[i].video_simulation = RECORDING_ON;

    strcpy(video_simulation_path,filename);
}

void RecordOne(char * filename)
{
  if (!VideoInputsOk()) return;

    if ( strlen( filename ) > 250 ) return;

    int i=0;
    for (i=0; i<total_cameras; i++)  camera_feeds[i].video_simulation = RECORDING_ONE_ON;

    strcpy(video_simulation_path,filename);
}

void Stop()
{
  if (!VideoInputsOk()) return;

     int i=0;
     for (i=0; i<total_cameras; i++)  camera_feeds[i].video_simulation = LIVE_ON;
}

unsigned int VideoSimulationState()
{
  if (!VideoInputsOk()) return NO_VIDEO_AVAILIABLE;
  return camera_feeds[0].video_simulation;
}
// SNAPSHOT RECORDING
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
