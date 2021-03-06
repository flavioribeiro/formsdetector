/* FormsDetector based on V4L2 video picture grabber

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include "libv4l2.h"

#define CLEAR(x) memset(&(x), 0, sizeof(x))

#define THRESHOLD 150 

struct buffer {
        char   *start;
        size_t length;
};

static void xioctl(int fh, int request, void *arg)
{
        int r;

        do {
                r = v4l2_ioctl(fh, request, arg);
        } while (r == -1 && ((errno == EINTR) || (errno == EAGAIN)));

        if (r == -1) {
                fprintf(stderr, "error %d, %s\n", errno, strerror(errno));
                exit(EXIT_FAILURE);
        }
}


void binarize(struct buffer *buffers, struct v4l2_buffer buf) {
    char out_name[256];
    FILE *fout, *ball, *square, *triangle, *saida;
    int i, sentinel, ball_sentinel =0, square_sentinel=0, triangle_sentinel=0;
    unsigned char rgb_frame[buf.bytesused], bw_frame[buf.bytesused/3];
    unsigned char ball_frame[buf.bytesused/3], square_frame[buf.bytesused/3], triangle_frame[buf.bytesused/3];
    unsigned char pixel[3];

    printf("binarizando\n");

    fout = fopen("retrato.pbm", "w");

    memcpy(rgb_frame, buffers[buf.index].start, buf.bytesused);

    for (i=0; i<=buf.bytesused; i=i+3) {

        pixel[0] = rgb_frame[i];
        pixel[1] = rgb_frame[i+1];
        pixel[2] = rgb_frame[i+2];

        if (pixel[0] > 255) pixel[0] = 255;
        if (pixel[0] <  0 ) pixel[0] = 0;

        if (pixel[1] > 255) pixel[1] = 255;
        if (pixel[1] <  0 ) pixel[1] = 0;

        if (pixel[2] > 255) pixel[2] = 255;
        if (pixel[2] <  0 ) pixel[2] = 0;

        if (pixel[0] > THRESHOLD) sentinel++;
        if (pixel[1] > THRESHOLD) sentinel++;
        if (pixel[2] > THRESHOLD) sentinel++;

        if (sentinel >= 2)
            fprintf(fout, "0");
        else
            fprintf(fout, "1");

        sentinel = 0;
    }

    fclose(fout);

    ball = fopen("ball.pbm", "r");
    square = fopen("square.pbm","r");
    triangle = fopen("triangle.pbm","r");
    saida = fopen("retrato.pbm","r");

    fread(ball_frame, buf.bytesused/3, 1, ball);
    fread(square_frame, buf.bytesused/3, 1, square);
    fread(triangle_frame, buf.bytesused/3, 1, triangle);
    fread(bw_frame, buf.bytesused/3, 1, saida);

    for(i=0; i<=buf.bytesused/3; i++) {
#if 0
        if ( bw_frame[1] == ball_frame[1]) ball_sentinel++;
        if ( bw_frame[1] == triangle_frame[1])  triangle_sentinel++;
        if ( bw_frame[1] == square_frame[1]) square_sentinel++;
#endif
#if 1
        if ((bw_frame[i] == '0') && (ball_frame[i]     == '0')) ball_sentinel++;
        if ((bw_frame[i] == '1') && (ball_frame[i]     == '1')) ball_sentinel++;
     //   if ((bw_frame[i] == '1') && (ball_frame[i]     == '0')) ball_sentinel--;
     //   if ((bw_frame[i] == '0') && (ball_frame[i]     == '1')) ball_sentinel--;
        
        if ((bw_frame[i] == '0') && (triangle_frame[i]   == '0')) triangle_sentinel++;
        if ((bw_frame[i] == '1') && (triangle_frame[i]   == '1')) triangle_sentinel++;
     //   if ((bw_frame[i] == '1') && (triangle_frame[i]   == '0')) triangle_sentinel--;
     //   if ((bw_frame[i] == '0') && (triangle_frame[i]   == '1')) triangle_sentinel--;

        if ((bw_frame[i] == '0') && (square_frame[i]     == '0')) square_sentinel++;
        if ((bw_frame[i] == '1') && (square_frame[i]     == '1')) square_sentinel++;
       // if ((bw_frame[i] == '1') && (square_frame[i]     == '0')) square_sentinel--;
     //   if ((bw_frame[i] == '0') && (square_frame[i]     == '1')) square_sentinel--;
#endif
   }
    fclose(ball);
    fclose(triangle);
    fclose(square);
    fclose(saida);

    ball = fopen("amostras/ball_mod.pbm","w");
    square = fopen("amostras/square_mod.pbm","w");
    triangle = fopen("amostras/triangle_mod.pbm","w");
    fout = fopen("amostras/saida.pbm","w");

    fprintf(square, "P1\n640 480 255\n");
    fwrite(square_frame, buf.bytesused/3, 1, square);
 
    fprintf(ball, "P1\n640 480 255\n");
    fwrite(ball_frame, buf.bytesused/3, 1, ball);
 
    fprintf(triangle, "P1\n640 480 255\n");
    fwrite(triangle_frame, buf.bytesused/3, 1, triangle);
 
    fprintf(fout, "P1\n640 480 255\n");
    fwrite(bw_frame, buf.bytesused/3, 1, fout);

    fclose(fout);
    fclose(square);
    fclose(ball);
    fclose(triangle);

    printf("É um circulo? %d\n", ball_sentinel);
    printf("É um triangulo? %d\n", triangle_sentinel);
    printf("É um quadrado? %d\n", square_sentinel);

    if ((ball_sentinel > triangle_sentinel) && \
            (ball_sentinel > square_sentinel))
            printf("\nEH UMA BOLA\n");

    else if ((triangle_sentinel > ball_sentinel) && \
            (triangle_sentinel > square_sentinel))
            printf("\nEH UM TRIANGULO\n");

    else if ((square_sentinel > ball_sentinel) && \
            (square_sentinel > triangle_sentinel))
            printf("\nEH UM QUADRADO\n");
}

int grab_frame()
{
        struct v4l2_format              fmt;
        struct v4l2_buffer              buf;
        struct v4l2_requestbuffers      req;
        enum v4l2_buf_type              type;
        fd_set                          fds;
        struct timeval                  tv;
        int                             r, fd = -1;
        unsigned int                    i, n_buffers;
        char                            *dev_name = "/dev/video1";
        struct buffer                   *buffers;

        fd = v4l2_open(dev_name, O_RDWR | O_NONBLOCK, 0);
        if (fd < 0) {
                perror("Cannot open device");
                exit(EXIT_FAILURE);
        }

        printf("grabbing frame...\n");
        CLEAR(fmt);
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width       = 640;
        fmt.fmt.pix.height      = 480;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
        fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
        xioctl(fd, VIDIOC_S_FMT, &fmt);
        if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB24) {
                printf("Libv4l didn't accept RGB24 format. Can't proceed.\n");
                exit(EXIT_FAILURE);
        }
        if ((fmt.fmt.pix.width != 640) || (fmt.fmt.pix.height != 480))
                printf("Warning: driver is sending image at %dx%d\n",
                        fmt.fmt.pix.width, fmt.fmt.pix.height);

        CLEAR(req);
        req.count = 100;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;
        xioctl(fd, VIDIOC_REQBUFS, &req);

        buffers = calloc(req.count, sizeof(*buffers));
        for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
                CLEAR(buf);

                buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory      = V4L2_MEMORY_MMAP;
                buf.index       = n_buffers;

                xioctl(fd, VIDIOC_QUERYBUF, &buf);

                buffers[n_buffers].length = buf.length;
                buffers[n_buffers].start = v4l2_mmap(NULL, buf.length,
                              PROT_READ | PROT_WRITE, MAP_SHARED,
                              fd, buf.m.offset);

                if (MAP_FAILED == buffers[n_buffers].start) {
                        perror("mmap");
                        exit(EXIT_FAILURE);
                }
        }

        for (i = 0; i < n_buffers; ++i) {
                CLEAR(buf);
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = i;
                xioctl(fd, VIDIOC_QBUF, &buf);
        }
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        xioctl(fd, VIDIOC_STREAMON, &type);
        for (i = 0; i < req.count; i++) {
                do {
                        FD_ZERO(&fds);
                        FD_SET(fd, &fds);

                        /* Timeout. */
                        tv.tv_sec = 2;
                        tv.tv_usec = 0;

                        r = select(fd + 1, &fds, NULL, NULL, &tv);
                } while ((r == -1 && (errno = EINTR)));
                if (r == -1) {
                        perror("select");
                        return errno;
                }

                CLEAR(buf);
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                xioctl(fd, VIDIOC_DQBUF, &buf);


                xioctl(fd, VIDIOC_QBUF, &buf);
        }

        binarize(buffers, buf);

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        xioctl(fd, VIDIOC_STREAMOFF, &type);
        for (i = 0; i < n_buffers; ++i)
                v4l2_munmap(buffers[i].start, buffers[i].length);
        v4l2_close(fd);

        return 0;
}

int main() {

    grab_frame();

    return 1;
}
