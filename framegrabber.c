/*
 *  V4L2 video capture example
 *
 *  This program can be used and distributed without restrictions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h>		/* getopt_long() */

#include <fcntl.h>		/* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/types.h>		/* for videodev2.h */

#include <linux/videodev2.h>

#define CLEAR(x) memset (&(x), 0, sizeof (x))

typedef enum
{
  IO_METHOD_READ,
  IO_METHOD_MMAP,
  IO_METHOD_USERPTR,
} io_method;

struct buffer
{
  void *start;
  size_t length;
};

static char *dev_name = "video0";
static io_method io = IO_METHOD_MMAP;
static int fd = -1;
struct buffer *buffers = NULL;
static unsigned int n_buffers = 0;
static struct v4l2_format fmt;
static int timeout = 1;

static void
errno_exit (const char *s)
{
  fprintf (stderr, "%s error %d, %s\n", s, errno, strerror (errno));

  exit (EXIT_FAILURE);
}

static int
xioctl (int fd, int request, void *arg)
{
  int r;

  do
    r = ioctl (fd, request, arg);
  while (-1 == r && EINTR == errno);

  return r;
}

//static void
//process_image (const void *p)
//{
//  fputc ('.', stdout);
//  fflush (stdout);
//}

static int
read_frame (void)
{
  struct v4l2_buffer buf;
  unsigned int i;

//  switch (io)
//    {
//    case IO_METHOD_READ:
//      if (-1 == read (fd, buffers[0].start, buffers[0].length))
//	{
//	  switch (errno)
//	    {
//	    case EAGAIN:
//	      return 0;
//
//	    case EIO:
	      /* Could ignore EIO, see spec. */

	      /* fall through */

//	    default:
//	      errno_exit ("read");
//	    }
//	}

  //      printf("io method read\n");
  //      process_image (buffers[0].start); //"output.ppm");

    //  break;

//    case IO_METHOD_MMAP:
      CLEAR (buf);

      buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory = V4L2_MEMORY_MMAP;

      if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf))
	{
	  switch (errno)
	    {
	    case EAGAIN:
	      return 0;

	    case EIO:
	      /* Could ignore EIO, see spec. */

	      /* fall through */

	    default:
	      errno_exit ("VIDIOC_DQBUF");
	    }
	}

      assert (buf.index < n_buffers);

      printf("io method mmap\n");
      process_image (buffers[buf.index].start);

      if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
	errno_exit ("VIDIOC_QBUF");

//      break;

//    case IO_METHOD_USERPTR:
//      CLEAR (buf);
//
  //    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    //  buf.memory = V4L2_MEMORY_USERPTR;
//
  //    if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf))
//	{
//	  switch (errno)
//	    {
//	    case EAGAIN:
//	      return 0;
//
//	    case EIO:
	      /* Could ignore EIO, see spec. */

	      /* fall through */

//	    default:
//	      errno_exit ("VIDIOC_DQBUF");
//	    }
//	}

  //    for (i = 0; i < n_buffers; ++i)
//	if (buf.m.userptr == (unsigned long) buffers[i].start
//	    && buf.length == buffers[i].length)
//	  break;

  //    assert (i < n_buffers);

//        printf("io method userptr\n");
  //    process_image ((void *) buf.m.userptr);

    //  if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
//	errno_exit ("VIDIOC_QBUF");

  //    break;
    //}

  return 1;
}

static void
mainloop (void)
{
  unsigned int count;

  count = timeout;

  printf("%d\n", count);
  while (count-- > 0)
    {
      for (;;)
	{
         printf(".");
	  fd_set fds;
	  struct timeval tv;
	  int r;

	  FD_ZERO (&fds);
	  FD_SET (fd, &fds);

	  /* Timeout. */
	  tv.tv_sec = 2;
	  tv.tv_usec = 0;

	  r = select (fd + 1, &fds, NULL, NULL, &tv);

	  if (-1 == r)
	    {
	      if (EINTR == errno)
		continue;

	      errno_exit ("select");
	    }

	  if (0 == r)
	    {
	      fprintf (stderr, "select timeout\n");
	      exit (EXIT_FAILURE);
	    }

            printf("read frame\n");
	  if (read_frame ())
	    break;

	  /* EAGAIN - continue select loop. */
	}
    }
}

static void
stop_capturing (void)
{
  enum v4l2_buf_type type;

  switch (io)
    {
    case IO_METHOD_READ:
      /* Nothing to do. */
      break;

    case IO_METHOD_MMAP:
    case IO_METHOD_USERPTR:
      type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

      if (-1 == xioctl (fd, VIDIOC_STREAMOFF, &type))
	errno_exit ("VIDIOC_STREAMOFF");

      break;
    }
}

static void
start_capturing (void)
{
  unsigned int i;
  enum v4l2_buf_type type;

  switch (io)
    {
    case IO_METHOD_READ:
      /* Nothing to do. */
      break;

    case IO_METHOD_MMAP:
      for (i = 0; i < n_buffers; ++i)
	{
	  struct v4l2_buffer buf;

	  CLEAR (buf);

	  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	  buf.memory = V4L2_MEMORY_MMAP;
	  buf.index = i;

	  if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
	    errno_exit ("VIDIOC_QBUF");
	}

      type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

      if (-1 == xioctl (fd, VIDIOC_STREAMON, &type))
	errno_exit ("VIDIOC_STREAMON");

      break;

    case IO_METHOD_USERPTR:
      for (i = 0; i < n_buffers; ++i)
	{
	  struct v4l2_buffer buf;

	  CLEAR (buf);

	  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	  buf.memory = V4L2_MEMORY_USERPTR;
	  buf.index = i;
	  buf.m.userptr = (unsigned long) buffers[i].start;
	  buf.length = buffers[i].length;

	  if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
	    errno_exit ("VIDIOC_QBUF");
	}

      type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

      if (-1 == xioctl (fd, VIDIOC_STREAMON, &type))
	errno_exit ("VIDIOC_STREAMON");

      break;
    }
}

static void
uninit_device (void)
{
  unsigned int i;

  switch (io)
    {
    case IO_METHOD_READ:
      free (buffers[0].start);
      break;

    case IO_METHOD_MMAP:
      for (i = 0; i < n_buffers; ++i)
	if (-1 == munmap (buffers[i].start, buffers[i].length))
	  errno_exit ("munmap");
      break;

    case IO_METHOD_USERPTR:
      for (i = 0; i < n_buffers; ++i)
	free (buffers[i].start);
      break;
    }

  free (buffers);
}

static void
init_read (unsigned int buffer_size)
{
  buffers = calloc (1, sizeof (*buffers));

  if (!buffers)
    {
      fprintf (stderr, "Out of memory\n");
      exit (EXIT_FAILURE);
    }

  buffers[0].length = buffer_size;
  buffers[0].start = malloc (buffer_size);

  if (!buffers[0].start)
    {
      fprintf (stderr, "Out of memory\n");
      exit (EXIT_FAILURE);
    }
}

static void
init_mmap (void)
{
  struct v4l2_requestbuffers req;

  CLEAR (req);

  req.count = 4;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;

  if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req))
    {
      if (EINVAL == errno)
	{
	  fprintf (stderr, "%s does not support "
		   "memory mapping\n", dev_name);
	  exit (EXIT_FAILURE);
	}
      else
	{
	  errno_exit ("VIDIOC_REQBUFS");
	}
    }

  if (req.count < 2)
    {
      fprintf (stderr, "Insufficient buffer memory on %s\n", dev_name);
      exit (EXIT_FAILURE);
    }

  buffers = calloc (req.count, sizeof (*buffers));

  if (!buffers)
    {
      fprintf (stderr, "Out of memory\n");
      exit (EXIT_FAILURE);
    }

  for (n_buffers = 0; n_buffers < req.count; ++n_buffers)
    {
      struct v4l2_buffer buf;

      CLEAR (buf);

      buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory = V4L2_MEMORY_MMAP;
      buf.index = n_buffers;

      if (-1 == xioctl (fd, VIDIOC_QUERYBUF, &buf))
	errno_exit ("VIDIOC_QUERYBUF");

      buffers[n_buffers].length = buf.length;
      buffers[n_buffers].start = mmap (NULL /* start anywhere */ ,
				       buf.length,
				       PROT_READ | PROT_WRITE /* required */ ,
				       MAP_SHARED /* recommended */ ,
				       fd, buf.m.offset);

      if (MAP_FAILED == buffers[n_buffers].start)
	errno_exit ("mmap");
    }
}

static void
init_userp (unsigned int buffer_size)
{
  struct v4l2_requestbuffers req;
  unsigned int page_size;

  page_size = getpagesize ();
  buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);

  CLEAR (req);

  req.count = 4;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_USERPTR;

  if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req))
    {
      if (EINVAL == errno)
	{
	  fprintf (stderr, "%s does not support "
		   "user pointer i/o\n", dev_name);
	  exit (EXIT_FAILURE);
	}
      else
	{
	  errno_exit ("VIDIOC_REQBUFS");
	}
    }

  buffers = calloc (4, sizeof (*buffers));

  if (!buffers)
    {
      fprintf (stderr, "Out of memory\n");
      exit (EXIT_FAILURE);
    }

  for (n_buffers = 0; n_buffers < 4; ++n_buffers)
    {
      buffers[n_buffers].length = buffer_size;
      buffers[n_buffers].start = memalign ( /* boundary */ page_size,
					   buffer_size);

      if (!buffers[n_buffers].start)
	{
	  fprintf (stderr, "Out of memory\n");
	  exit (EXIT_FAILURE);
	}
    }
}

static void
init_device (void)
{
  struct v4l2_capability cap;
  struct v4l2_cropcap cropcap;
  struct v4l2_crop crop;
  unsigned int min;

  if (-1 == xioctl (fd, VIDIOC_QUERYCAP, &cap))
    {
      if (EINVAL == errno)
	{
	  fprintf (stderr, "%s is no V4L2 device\n", dev_name);
	  exit (EXIT_FAILURE);
	}
      else
	{
	  errno_exit ("VIDIOC_QUERYCAP");
	}
    }

  if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
      fprintf (stderr, "%s is no video capture device\n", dev_name);
      exit (EXIT_FAILURE);
    }

  switch (io)
    {
    case IO_METHOD_READ:
      if (!(cap.capabilities & V4L2_CAP_READWRITE))
	{
	  fprintf (stderr, "%s does not support read i/o\n", dev_name);
	  exit (EXIT_FAILURE);
	}

      break;

    case IO_METHOD_MMAP:
    case IO_METHOD_USERPTR:
      if (!(cap.capabilities & V4L2_CAP_STREAMING))
	{
	  fprintf (stderr, "%s does not support streaming i/o\n", dev_name);
	  exit (EXIT_FAILURE);
	}

      break;
    }


  /* Select video input, video standard and tune here. */


  CLEAR (cropcap);

  cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if (0 == xioctl (fd, VIDIOC_CROPCAP, &cropcap))
    {
      crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      crop.c = cropcap.defrect;	/* reset to default */

      if (-1 == xioctl (fd, VIDIOC_S_CROP, &crop))
	{
	  switch (errno)
	    {
	    case EINVAL:
	      /* Cropping not supported. */
	      break;
	    default:
	      /* Errors ignored. */
	      break;
	    }
	}
    }
  else
    {
      /* Errors ignored. */
    }


  CLEAR (fmt);

  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width = 640;
  fmt.fmt.pix.height = 480;
  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
  fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

  if (-1 == xioctl (fd, VIDIOC_S_FMT, &fmt))
    errno_exit ("VIDIOC_S_FMT");

  /* Note VIDIOC_S_FMT may change width and height. */

  /* Buggy driver paranoia. */
  min = fmt.fmt.pix.width * 2;
  if (fmt.fmt.pix.bytesperline < min)
    fmt.fmt.pix.bytesperline = min;
  min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
  if (fmt.fmt.pix.sizeimage < min)
    fmt.fmt.pix.sizeimage = min;

  switch (io)
    {
    case IO_METHOD_READ:
      init_read (fmt.fmt.pix.sizeimage);
      break;

    case IO_METHOD_MMAP:
      init_mmap ();
      break;

    case IO_METHOD_USERPTR:
      init_userp (fmt.fmt.pix.sizeimage);
      break;
    }
}

static void
close_device (void)
{
  if (-1 == close (fd))
    errno_exit ("close");

  fd = -1;
}

static void
open_device (void)
{
  struct stat st;

  if (-1 == stat (dev_name, &st))
    {
      fprintf (stderr, "Cannot identify '%s': %d, %s\n",
	       dev_name, errno, strerror (errno));
      exit (EXIT_FAILURE);
    }

  if (!S_ISCHR (st.st_mode))
    {
      fprintf (stderr, "%s is no device\n", dev_name);
      exit (EXIT_FAILURE);
    }

  fd = open (dev_name, O_RDWR /* required */  | O_NONBLOCK, 0);

  if (-1 == fd)
    {
      fprintf (stderr, "Cannot open '%s': %d, %s\n",
	       dev_name, errno, strerror (errno));
      exit (EXIT_FAILURE);
    }
}

int process_image(const void *p)
{

    static int packed_value, i;
    FILE* fp = fopen("output.ppm", "w" );

    // Write PNM header
    fprintf( fp, "P6\n" );
    fprintf( fp, "# YUV422 frame -> RGB \n" );
    fprintf( fp, "%d %d\n", fmt.fmt.pix.width, fmt.fmt.pix.height);
    
    // fprintf( stderr, "&userframe address %d\n", &userframe );
    // fprintf( stderr, "&p address %d\n", &p );
    // fprintf( stderr, "sizeof(userframe) %d\n", sizeof(userframe) );
    
//     switch (userfmt.fmt.pix.pixelformat){
//     case V4L2_PIX_FMT_RGB24:
//         
//         // Max val
//         // NOTE UNTESTED!!!!!!!!!
//         fprintf( fp, "255\n" );
//         fprintf( stderr, "frame_save(): RGB24 Unsupported type!\n" );
// 
//         //Write image data
//         for ( i = 0; i < ( userfmt.fmt.pix.width * userfmt.fmt.pix.height ); i++ )
//         {
//         // 3 bytes per pixel
// //         rgb = ((FRAME_RGB*)(p))[i];
// // 
// //         fprintf( fp, "%c%c%c",
// //              rgb.blue,
// //              rgb.green,
// //              rgb.red );
//         }
//         break;
//         
//     case V4L2_PIX_FMT_RGB32:
//         // NOTE UNTESTED!!!!!!!!!
//         // Max val
//         fprintf( fp, "255\n" );
//         
//         // Write image data
//         for ( i = 0; i < ( userfmt.fmt.pix.width * userfmt.fmt.pix.height ); i++ )
//         {
//         // Retrieve lower 24 bits of ARGB
//         packed_value = ((int*)(p))[i] & 0x00ffffff;
//         
//         fprintf( fp, "%c%c%c",
//              ( packed_value & 0x00ff0000 ) >> 16, // Blue
//              ( packed_value & 0x0000ff00 ) >>  8, // Green
//              ( packed_value & 0x000000ff )        // Red
//              );
//         }
//         break;
//         
//     case V4L2_PIX_FMT_RGB565:
//     case V4L2_PIX_FMT_RGB555:
//         // NOTE UNTESTED!!!!!!!!!
//         // Max val
//         fprintf( fp, "65535\n" );
//         
//         // Write image data
//         for ( i = 0; i < ( userfmt.fmt.pix.width *userfmt.fmt.pix.height ); i++ ){
//         // Retrieve 16-bit words
//         packed_value = ((short*)(p))[i];
//         
//         fprintf( fp, "%c%c",frame
//              ( packed_value & 0xff00 ) >> 8, // High
//              ( packed_value & 0x00ff )       // Low
//              );
//             }
//         break;
// 
//     case V4L2_PIX_FMT_YUYV:
        int Y0, Y1, Cb, Cr;            /* gamma pre-corrected input [0;255] */
        int ER0,ER1,EG0,EG1,EB0,EB1;    /* output [0;255] */
        double r0,r1,g0,g1,b0,b1;             /* temporaries */
        double y0,y1, pb, pr;

        // Max val
        fprintf( fp, "255\n" );
        //fprintf( stderr, "frame_save(): YUYV file type!\n" );

        while(i < (fmt.fmt.pix.width * fmt.fmt.pix.height/2)){

        packed_value = *((int*)p+i);

        Y0 = (char)(packed_value & 0xFF);
        Cb = (char)((packed_value >> 8) & 0xFF);
        Y1 = (char)((packed_value >> 16) & 0xFF);
        Cr = (char)((packed_value >> 24) & 0xFF);

        // Strip sign values after shift (i.e. unsigned shift)
        Y0 = Y0 & 0xFF;
        Cb = Cb & 0xFF;
        Y1 = Y1 & 0xFF;
        Cr = Cr & 0xFF;

        //fprintf( fp, "Value:%x Y0:%x Cb:%x Y1:%x Cr:%x ",packed_value,Y0,Cb,Y1,Cr);

        y0 = (255 / 219.0) * (Y0 - 16);
        y1 = (255 / 219.0) * (Y1 - 16);
        pb = (255 / 224.0) * (Cb - 128);
        pr = (255 / 224.0) * (Cr - 128);

        // Generate first pixel
        r0 = 1.0 * y0 + 0     * pb + 1.402 * pr;
        g0 = 1.0 * y0 - 0.344 * pb - 0.714 * pr;
        b0 = 1.0 * y0 + 1.772 * pb + 0     * pr;

        // Generate next pixel - must reuse pb & pr as 4:2:2
        r1 = 1.0 * y1 + 0     * pb + 1.402 * pr;
        g1 = 1.0 * y1 - 0.344 * pb - 0.714 * pr;
        b1 = 1.0 * y1 + 1.772 * pb + 0     * pr;

        if (r0 > 255) ER0 = 255;
        if (r1 > 255) ER1 = 255;
        if (g0 > 255) EG0 = 255;
        if (g1 > 255) EG1 = 255;
        if (b0 > 255) EB0 = 255;
        if (b1 > 255) EB1 = 255;
//        ER0 = clamp (r0);
 //       ER1 = clamp (r1);
   //     EG0 = clamp (g0);
     //   EG1 = clamp (g1);
       // EB0 = clamp (b0);
       // EB1 = clamp (b1);

        fprintf( fp, "%c%c%c%c%c%c",ER0,EG0,EB0,ER1,EG1,EB1); // Output two pixels
//        fprintf( stdout, "R:%d G:%d B:%d     R:%d G:%d B:%d \n",ER0,EG0,EB0,ER1,EG1,EB1);

        i++;
        }

        //fprintf( stderr, "Size of packed_value:%d Y0:%d Cb:%d Cr:%d Y1:%d\n", sizeof(packed_value), sizeof(y0), sizeof(cb0), sizeof(y1), sizeof(cr0));

//        break;
// 
//     default:
//         // Unsupported!
//         fprintf( stderr, "frame_save(): Unsupported type!\n" );
//         return -1;
//         }

    fprintf( stderr, "frame saved\n" );
    fclose( fp );
    return 0;
}

int
main (int argc, char **argv)
{
  dev_name = "/dev/video0";

  printf("open device\n");
  open_device ();

  printf("init device\n");
  init_device ();

  printf("start capturing \n");
  start_capturing ();

  printf("mainloop\n");
  mainloop ();

  printf("stop capturing\n");
  stop_capturing ();

  printf("uninit device\n");
  uninit_device ();

  printf("close device\n");
  close_device ();

  exit (EXIT_SUCCESS);

  return 0;
}



