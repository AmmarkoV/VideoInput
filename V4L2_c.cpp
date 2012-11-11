#include "V4L2_c.h"


#include <string.h>
#include <assert.h>

#include <getopt.h>             /* getopt_long() */

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/types.h>          /* for videodev2.h */


int populate_v4l2_interface(struct V4L2_c_interface * v4l2_interface,char * device,int method_used)
{
  if (method_used==MMAP) { v4l2_interface->io=IO_METHOD_MMAP; } else
                         { v4l2_interface->io=IO_METHOD_MMAP; } /*If method used is incorrect just use MMAP :P*/
  v4l2_interface->fd=-1;
  v4l2_interface->buffers=0;
  v4l2_interface->n_buffers= 0;

  strncpy(v4l2_interface->device,device,MAX_DEVICE_FILENAME);
  fprintf(stderr,"opening device: %s\n",device);

  struct stat st;
  if (-1 == stat (device, &st)) { fprintf (stderr, "Cannot identify '%s': %d, %s\n",device, errno, strerror (errno)); return 0;  }

  if (!S_ISCHR (st.st_mode))    { fprintf (stderr, "%s is no device\n", device); return 0; }

  v4l2_interface->fd = open (device, O_RDWR /* required */ | O_NONBLOCK, 0);

  if (-1 == v4l2_interface->fd)
   {
        fprintf (stderr, "Cannot open '%s': %d, %s\n",device, errno, strerror (errno));
        return 0;
   }

   fprintf(stderr,"device opening ok \n");
   return 1;
}
