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
#include "image_storage.h"
#include <unistd.h>

#define LIVE_ON 0
#define RECORDING_ON 1
#define RECORDING_ONE_ON 2
#define PLAYBACK_ON 3
#define PLAYBACK_ON_LOADED 4
#define WORKING 5

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

  /*VIDEO SIMULATION DATA*/
  struct Image rec_video;
  int video_simulation;

  /* THREADING DATA */
  int thread_alive_flag;
  int snap_lock;
  int stop_snap_loop;
  pthread_t loop_thread;
};

struct ThreadPassParam
{
    int feednum;
};


int total_cameras=0;
struct Video * camera_feeds=0;
char video_simulation_path[256];
io_method io=IO_METHOD_MMAP; //IO_METHOD_MMAP; // IO_METHOD_READ; //IO_METHOD_USERPTR;


void * SnapLoop(void *ptr );


char * VIDEOINPT_VERSION=(char *) "0.15";

char * VideoInput_Version()
{
  return VIDEOINPT_VERSION;
}


char FileExists(char * filename)
{
FILE *fp = fopen(filename,"r");
 if( fp ) { // exists
            fclose(fp);
            return 1;
          }
          else
          {
           // doesnt exist
          }
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


int InitVideoFeed(int inpt,char * viddev,int width,int height,char snapshots_on)
{
   printf("Initializing Video Feed %u ( %s ) @ %u/%u \n",inpt,viddev,width,height);
   if (!VideoInputsOk()) return 0;
   if ( (!FileExists(viddev)) ) { fprintf(stderr,"Super Quick linux check for the webcam (%s) returned false.. please connect V4L2 compatible camera!\n",viddev); return 0; }

   camera_feeds[inpt].videoinp = viddev; //px (char *) "/dev/video0";
   camera_feeds[inpt].width = width;
   camera_feeds[inpt].height = height;
   camera_feeds[inpt].size_of_frame=width*height*3;
   camera_feeds[inpt].video_simulation=LIVE_ON;
   camera_feeds[inpt].thread_alive_flag=0;
   camera_feeds[inpt].snap_lock=0;

     CLEAR (camera_feeds[inpt].fmt);
     camera_feeds[inpt].fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
     camera_feeds[inpt].fmt.fmt.pix.width       = width;
     camera_feeds[inpt].fmt.fmt.pix.height      = height;
     camera_feeds[inpt].fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_VYUY; //V4L2_PIX_FMT_RGB24; //V4L2_PIX_FMT_YUV420;
     camera_feeds[inpt].fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;


      camera_feeds[inpt].v4l2_intf = new V4L2(camera_feeds[inpt].videoinp, io);
       if ( camera_feeds[inpt].v4l2_intf->set(camera_feeds[inpt].fmt) == 0 ) { fprintf(stderr,"Device does not support settings:\n"); return 0; }
         else
       {
           fprintf(stderr,"No errors , starting camera %u / locking memory..!",inpt);
           camera_feeds[inpt].v4l2_intf->initBuffers();
           camera_feeds[inpt].v4l2_intf->startCapture();

           camera_feeds[inpt].frame = 0;
           //camera_feeds[inpt].frame = malloc(width*height*3);
           //if ( camera_feeds[inpt].frame == 0 ) { fprintf(stderr,"Cannot Allocate memory for frame #%u!\n",inpt); return 0; }
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
 return 1;
}

int UnpauseFeed(int feednum)
{
 if (!VideoInputsOk()) return 0;
 //camera_feeds[feednum].snap_lock=0;
 return 1;
}


unsigned char * GetFrame(int webcam_id)
{
  if (!VideoInputsOk()) return 0;

  switch(camera_feeds[webcam_id].video_simulation)
  {
    case  LIVE_ON :
      if (total_cameras>webcam_id) {  return (unsigned char *) camera_feeds[webcam_id].frame;} else
                                   { return 0; }

    break;

    case  PLAYBACK_ON_LOADED :
      if (total_cameras>webcam_id) {  return (unsigned char *) camera_feeds[webcam_id].rec_video.pixels;} else
                                   { return 0; }

    break;

    case  PLAYBACK_ON :
     fprintf(stderr,"Reading Snapshot\n");

      char store_path[256]={0};
      char last_part[6]="0.raw";
      last_part[0]='0'+webcam_id;

      strcpy(store_path,video_simulation_path);
      strcat(store_path,last_part);
      ReadPPM(store_path,&camera_feeds[webcam_id].rec_video);
      camera_feeds[webcam_id].video_simulation = PLAYBACK_ON_LOADED;
      fprintf(stderr,"Returning Snapshot\n");
      return (unsigned char *) camera_feeds[webcam_id].rec_video.pixels;
    break;

  }
  return 0;
}



void RecordInLoop(int feed_num)
{
    unsigned int mode_started = camera_feeds[feed_num].video_simulation;
    camera_feeds[feed_num].video_simulation = WORKING;

    memcpy(camera_feeds[feed_num].rec_video.pixels,camera_feeds[feed_num].frame,camera_feeds[feed_num].size_of_frame);

    char store_path[256]={0};
    char last_part[6]="0.ppm";
    last_part[0]='0'+feed_num;

    strcpy(store_path,video_simulation_path);
    strcat(store_path,last_part);
    WritePPM(store_path,&camera_feeds[feed_num].rec_video);
    if ( mode_started == RECORDING_ONE_ON) { camera_feeds[feed_num].video_simulation = LIVE_ON; }

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
       usleep(15); /* 20ms sleep time per sample , its a good value for 2 cameras*/

       if ( camera_feeds[feed_num].snap_lock == 0 )
       { // WE DONT NEED THE SNAPSHOT TO BE LOCKED!
          camera_feeds[feed_num].frame=camera_feeds[feed_num].v4l2_intf->getFrame();
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
   camera_feeds[feed_num].thread_alive_flag=0;
   return ( void * ) 0 ;
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// SNAPSHOT RECORDING
void Play(char * filename)
{
    if (!VideoInputsOk()) return;
    if ( strlen( filename ) > 250 ) return;

    int i=0;
    for (i=0; i<total_cameras; i++)  camera_feeds[i].video_simulation = PLAYBACK_ON;

    strcpy(video_simulation_path,filename);
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
  if (!VideoInputsOk()) return 0;
  return camera_feeds[0].video_simulation;
}
// SNAPSHOT RECORDING
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
