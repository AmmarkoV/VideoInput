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

  /* THREADING DATA */
  int snap_lock;
  int stop_snap_loop;
  pthread_t loop_thread;
};

struct ThreadPassParam
{
    int feednum;
};

//int stop_snap_loop=0;
int total_cameras=0;
struct Video * camera_feeds=0;
char video_simulation_path[256];
int video_simulation=0;
io_method io=IO_METHOD_MMAP; //IO_METHOD_MMAP; // IO_METHOD_READ; //IO_METHOD_USERPTR;
//struct Video feed1,feed2;
//struct Image rec_left={0},rec_right={0};
//pthread_t loop_thread;

void * SnapLoop(void *ptr );


char * VIDEOINPT_VERSION="0.02";

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

int InitVideoInputs(int numofinputs)
{
    if (total_cameras>0) { fprintf(stderr,"Error , Video Inputs already active ?\n total_cameras=%u\n",total_cameras); return 0;}

    //First allocate memory for V4L2 Structures  , etc
    camera_feeds = (struct Video * ) malloc ( sizeof( struct Video ) * numofinputs );
    if (camera_feeds==0) { fprintf(stderr,"Error , cannot allocate memory for %u video inputs \n",total_cameras); return 0;}


    video_simulation = LIVE_ON;

    //Lets Refresh USB devices list :)
    system("lsusb");

    //We want higher priority now..! :)
    if ( nice(-4) == -1 ) { fprintf(stderr,"Error increasing priority on main video capture loop\n");} else
                          { fprintf(stderr,"Increased priority \n"); }


    total_cameras=numofinputs;
    usleep(30);

    return 1 ;
}

int CloseVideoInputs()
{
    if (total_cameras==0) { fprintf(stderr,"Error , Video Inputs already deactivated ?\n"); return 0;}
    if (camera_feeds==0) { fprintf(stderr,"Error , Video Inputs already deactivated ?\n"); return 0;}

    int i=0;
    for ( i=0; i<total_cameras; i++ )
     {
       fprintf(stderr,"Video %u Stopping\n",i);
       camera_feeds[i].stop_snap_loop=1;
       pthread_join( camera_feeds[i].loop_thread, NULL);
       usleep(30);
       camera_feeds[i].v4l2_intf->stopCapture();
       camera_feeds[i].v4l2_intf->freeBuffers();
       if ( camera_feeds[i].rec_video.pixels !=0 ) free( camera_feeds[i].rec_video.pixels );
     }



    fprintf(stderr,"Deallocation of Video Structures\n");

    free(camera_feeds);

    fprintf(stderr,"Video Input successfully deallocated\n");

    return 1 ;
}


int InitVideoFeed(int inpt,char * viddev,int width,int height,char snapshots_on)
{
   printf("Initializing Video Feed %u ( %s ) @ %u/%u \n",inpt,viddev,width,height);
   if ( (!FileExists(viddev)) ) { fprintf(stderr,"Super Quick linux check for the webcam (%s) returned false.. please connect V4L2 compatible camera!\n",viddev); return 0; }
   //int width=320,height=240;
   camera_feeds[inpt].videoinp = viddev; // (char *) "/dev/video0";
   camera_feeds[inpt].width = width;
   camera_feeds[inpt].height = height;
   camera_feeds[inpt].size_of_frame=width*height*3;
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

           camera_feeds[inpt].frame = malloc(width*height*3);
           if ( camera_feeds[inpt].frame == 0 ) { fprintf(stderr,"Cannot Allocate memory for frame #%u!\n",inpt); return 0; }
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


    camera_feeds[inpt].stop_snap_loop=0;

    struct ThreadPassParam param;
    param.feednum=inpt;
    pthread_create( &camera_feeds[inpt].loop_thread, NULL,  SnapLoop ,(void*) &param);

    sleep(1);
    printf("InitVideoFeed %u is ok!\n",inpt);



  return 1;
}




unsigned char * GetFrame(int webcam_id)
{
  switch(video_simulation)
  {
    case  LIVE_ON :
    if (total_cameras>webcam_id) {  return (unsigned char *) camera_feeds[webcam_id].frame;} else { return 0; }

    //if ( webcam_id == 1 ) { return (unsigned char *) feed1.frame; } else
    //if ( webcam_id == 2 ) { return (unsigned char *) feed2.frame; }
    break;

    case  PLAYBACK_ON_LOADED :
    if (total_cameras>webcam_id) {  return (unsigned char *) camera_feeds[webcam_id].rec_video.pixels;} else { return 0; }
    //if ( webcam_id == 1 ) { return (unsigned char *) rec_left.pixels; } else
    //if ( webcam_id == 2 ) { return (unsigned char *) rec_right.pixels; }
    break;
/*
    case  PLAYBACK_ON :
     fprintf(stderr,"Reading Snapshot\n");
           char store_path[256]={0};
           strcpy(store_path,video_simulation_path);
           ReadDoubleRAW(store_path,&rec_left,&rec_right);
           video_simulation = PLAYBACK_ON_LOADED;
     fprintf(stderr,"Returning Snapshot\n");
     if ( webcam_id == 1 ) { return (unsigned char *) rec_left.pixels; } else
     if ( webcam_id == 2 ) { return (unsigned char *) rec_right.pixels; }
    break;*/

  }
  return 0;
}

void * SnapLoop( void * ptr)
{
    printf("Starting SnapLoop!\n");
    struct ThreadPassParam *param=0;
    param = (struct ThreadPassParam *) ptr;
    int feed_num=param->feednum;

    //feed_num=*tmp_feed_num;

   //We want higher priority now..! :)
   if ( nice(-4) == -1 ) { fprintf(stderr,"Error increasing priority on main video capture loop\n");}else
                         { fprintf(stderr,"Increased priority \n"); }


    printf("Video capture thread #%u is alive \n",feed_num);

   while ( camera_feeds[feed_num].stop_snap_loop == 0 )
    {
       //usleep(10);
       camera_feeds[feed_num].frame=camera_feeds[feed_num].v4l2_intf->getFrame();
      // printf(".%u.",feed_num);
/*
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
*/
    }

    printf("Video capture thread #%u is closing..! \n",feed_num);

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
