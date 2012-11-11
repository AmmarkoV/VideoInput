#ifndef V4L2_C_H_INCLUDED
#define V4L2_C_H_INCLUDED

#include <linux/types.h>
#include <linux/videodev2.h>
#include "PrintV4L2.h"

#define CLEAR(x) memset (&(x), 0, sizeof (x))

#define MAX_DEVICE_FILENAME 256

#define TIMEOUT_SEC 10
#define TIMEOUT_USEC 0


enum v4l2_method_used
{
    READ ,
    MMAP ,
    USERPTR
};

typedef enum
{
  IO_METHOD_READ,
  IO_METHOD_MMAP,
  IO_METHOD_USERPTR,
} io_method;


struct buffer
{
  void * start;
  size_t length;
};

struct V4L2_c_interface
{
  char device[MAX_DEVICE_FILENAME];
  io_method io;
  int fd;
  buffer *buffers;
  unsigned int n_buffers;
};

static int xioctl  (int   fd, int   request,  void *  arg);

int populate_v4l2intf(struct V4L2_c_interface * v4l2_interface,char * device,int method_used);
int destroy_v4l2intf(struct V4L2_c_interface * v4l2_interface);
int getFileDescriptor_v4l2intf(struct V4L2_c_interface * v4l2_interface);


#endif // V4L2_C_H_INCLUDED
