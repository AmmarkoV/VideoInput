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

struct Video
{
  char * videoinp;

  struct v4l2_format fmt;
  void *frame;
  unsigned int size_of_frame;
  V4L2 *v4l2_intf;

  unsigned int height;
  unsigned int width;

  int snap_lock;
};


int stop_snap_loop=0;
char video_simulation_path[256];
int video_simulation=0;
io_method io=IO_METHOD_MMAP; //IO_METHOD_MMAP; // IO_METHOD_READ; //IO_METHOD_USERPTR;
struct Video feed1,feed2;
struct Image rec_left={0},rec_right={0};
pthread_t loop_thread;

void * SnapLoop(void *ptr );


char * VIDEOINPT_VERSION="0.01";

char * VideoInput_Version()
{
  return VIDEOINPT_VERSION;
}


char FileExists(char * filename)
{
FILE *fp = fopen(filename,"r");
 if( fp ) { // exists
            return 1;
            fclose(fp);
          }
          else
          {
           // doesnt exist
          }
 return 0;
}

int InitVideoInput(char * vid1 , char * vid2,char snapshots_on)
{
   // TRY TO INITIALIZE BEST POSSIBLE IMAGE SIZE
   // SHOULD ALSO TRY TO BENCHMARK SYSTEM IN ORDER TO DROP RESOLUTION IF NEEDED!
   video_simulation = LIVE_ON;
   //Super Quick linux check for the webcams :)
   if ( (!FileExists(vid1)) || (!FileExists(vid2) ) ) { fprintf(stderr,"Super Quick linux check for the webcams returned false.. please connect cameras!\n"); return 0; }

   //We want higher priority now..! :)
   if ( nice(-4) == -1 ) { fprintf(stderr,"Error increasing priority on main video capture loop\n");} else
                         { fprintf(stderr,"Increased priority \n"); }

   // WEBCAM 1 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
   int width=320,height=240;
   feed1.videoinp = vid1; // (char *) "/dev/video0";
   feed1.width = width;
   feed1.height = height;
   feed1.size_of_frame=width*height*3;
     CLEAR (feed1.fmt);
     feed1.fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
     feed1.fmt.fmt.pix.width       = width;
     feed1.fmt.fmt.pix.height      = height;
     feed1.fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_VYUY; //V4L2_PIX_FMT_RGB24; //V4L2_PIX_FMT_YUV420;
     feed1.fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;


      feed1.v4l2_intf = new V4L2(feed1.videoinp, io);
       if ( feed1.v4l2_intf->set(feed1.fmt) == 0 ) { fprintf(stderr,"device does not support settings:\n"); return 0; }
         else
       {
           fprintf(stderr,"No errors , starting camera 1 / locking memory..!");
           feed1.v4l2_intf->initBuffers();
           feed1.v4l2_intf->startCapture();

           feed1.frame = malloc(width*height*3);
           if ( feed1.frame == 0 ) { fprintf(stderr,"Cannot Allocate memory for frame #1!\n"); return 0; }
       }
    // WEBCAM 1 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

  // WEBCAM 2 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
   feed2.videoinp = vid2; // (char *) "/dev/video1";
   feed2.width = width;
   feed2.height = height;
   feed2.size_of_frame=width*height*3;
     CLEAR (feed2.fmt);
     feed2.fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
     feed2.fmt.fmt.pix.width       = width;
     feed2.fmt.fmt.pix.height      = height;
     feed2.fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_VYUY; //V4L2_PIX_FMT_RGB24; //V4L2_PIX_FMT_YUV420;
     feed2.fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;


      feed2.v4l2_intf = new V4L2(feed2.videoinp, io);
       if ( feed2.v4l2_intf->set(feed2.fmt) == 0 ) { fprintf(stderr,"device does not support settings:\n"); return 0; }
         else
       {
           fprintf(stderr,"No errors , starting camera 1 / locking memory..!");
           feed2.v4l2_intf->initBuffers();
           feed2.v4l2_intf->startCapture();

           feed2.frame = malloc(width*height*3);
           if ( feed2.frame == 0 ) { fprintf(stderr,"Cannot Allocate memory for frame #1!\n"); return 0; }
       }
    // WEBCAM 2 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


   // INIT MEMORY FOR SNAPSHOTS !
   rec_left.pixels = 0;  // Xreiazontai etsi wste an den theloume snapshots na min crasharei to sympan
   rec_right.pixels = 0; // Xreiazontai etsi wste an den theloume snapshots na min crasharei to sympan
    rec_left.size_x=width , rec_right.size_x=width;
    rec_left.size_y=height , rec_right.size_y=height;
    rec_left.depth=3 , rec_right.depth=3;
   if ( snapshots_on == 1 )
    {
        rec_left.pixels = (char * ) malloc( feed1.size_of_frame + 1);
        rec_right.pixels = (char * ) malloc( feed2.size_of_frame + 1);
    }
    // INIT MEMORY FOR SNAPSHOTS !

   sleep(1);

   stop_snap_loop=0;
   char *message1 = (char *)  "Thread 1";
   pthread_create( &loop_thread, NULL,  SnapLoop ,(void*) message1);

   return 1;
}


int CloseVideoInput()
{
    fprintf(stderr,"Closing Video Input\n");
    stop_snap_loop=1;
    pthread_join( loop_thread, NULL);
    usleep(30);


    fprintf(stderr,"Video 1 Stopping\n");
    feed1.v4l2_intf->stopCapture();
    feed1.v4l2_intf->freeBuffers();
    //if ( feed1.frame != 0 ) free( feed1.frame ); // to apeleytherwnei i parapanw call

    fprintf(stderr,"Video 2 Stopping\n");
    feed2.v4l2_intf->stopCapture();
    feed2.v4l2_intf->freeBuffers();
    //if ( feed2.frame != 0 ) free( feed2.frame ); // to apeleytherwnei i parapanw call

    fprintf(stderr,"Freeing Snapshots\n");
   // FREE MEMORY FOR SNAPSHOTS !
    if ( rec_left.pixels !=0 ) free( rec_left.pixels );
    if ( rec_right.pixels !=0 ) free( rec_right.pixels );
   // FREE MEMORY FOR SNAPSHOTS !
    fprintf(stderr,"Video Input Closed\n");
    return 0;
}


unsigned char * GetFrame(int webcam_id)
{
  switch(video_simulation)
  {
    case  LIVE_ON :
    if ( webcam_id == 1 ) { return (unsigned char *) feed1.frame; } else
    if ( webcam_id == 2 ) { return (unsigned char *) feed2.frame; }
    break;

    case  PLAYBACK_ON_LOADED :
    if ( webcam_id == 1 ) { return (unsigned char *) rec_left.pixels; } else
    if ( webcam_id == 2 ) { return (unsigned char *) rec_right.pixels; }
    break;

    case  PLAYBACK_ON :
     fprintf(stderr,"Reading Snapshot\n");
           char store_path[256]={0};
           strcpy(store_path,video_simulation_path);
           ReadDoubleRAW(store_path,&rec_left,&rec_right);
           video_simulation = PLAYBACK_ON_LOADED;
     fprintf(stderr,"Returning Snapshot\n");
     if ( webcam_id == 1 ) { return (unsigned char *) rec_left.pixels; } else
     if ( webcam_id == 2 ) { return (unsigned char *) rec_right.pixels; }
    break;

  }
  return 0;
}

void * SnapLoop( void * ptr)
{
   unsigned char turn=1;
    //We want higher priority now..! :)
   if ( nice(-4) == -1 ) { fprintf(stderr,"Error increasing priority on main video capture loop\n");}else
                         { fprintf(stderr,"Increased priority \n"); }

   while ( stop_snap_loop == 0 )
    {
       //usleep(10);
       if ( turn == 0 )
       {
         feed2.frame=feed2.v4l2_intf->getFrame();
         feed1.frame=feed1.v4l2_intf->getFrame();
         turn = 1;
       } else
       {
         feed1.frame=feed1.v4l2_intf->getFrame();
         feed2.frame=feed2.v4l2_intf->getFrame();
         turn = 0;
       }

  //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
       // SNAPSHOT RECORDING
       if ( ( feed1.frame == 0 ) || ( feed2.frame == 0 ) ) { fprintf(stderr,"Do not want Null frames recorded :P \n"); } else
       if ( (video_simulation == RECORDING_ON ) || (video_simulation == RECORDING_ONE_ON) )
       {
           unsigned int mode_started = video_simulation;
           video_simulation = WORKING;

           memcpy(rec_left.pixels,feed1.frame,feed1.size_of_frame);
           memcpy(rec_right.pixels,feed2.frame,feed2.size_of_frame);

           char store_path[256]={0};
           strcpy(store_path,video_simulation_path);
           WriteDoubleRAW(store_path,&rec_left,&rec_right);
           if ( mode_started == RECORDING_ONE_ON) { video_simulation = LIVE_ON; }
       }
       // SNAPSHOT RECORDING
  //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

    }
   return ( void * ) 0 ;
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// SNAPSHOT RECORDING
void Play(char * filename)
{
    if ( strlen( filename ) > 250 ) return;
    video_simulation = PLAYBACK_ON;
    strcpy(video_simulation_path,filename);
}

void Record(char * filename)
{
    if ( strlen( filename ) > 250 ) return;
    video_simulation = RECORDING_ON;
    strcpy(video_simulation_path,filename);
}

void RecordOne(char * filename)
{
    if ( strlen( filename ) > 250 ) return;
    video_simulation = RECORDING_ONE_ON;
    strcpy(video_simulation_path,filename);
}

void Stop()
{
     video_simulation = LIVE_ON;
}

unsigned int VideoSimulationState()
{
  return video_simulation;
}
// SNAPSHOT RECORDING
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
