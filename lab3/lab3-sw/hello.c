/*
 * Userspace program that communicates with the vga_ball device driver
 * through ioctls
 *
 * Stephen A. Edwards
 * Columbia University
 */

#include <stdio.h>
#include "vga_ball.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define BALL_WIDTH 32
#define BALL_HEIGHT 32

#define X_MIN 0
#define Y_MIN 0
#define X_MAX (SCREEN_WIDTH - BALL_WIDTH)
#define Y_MAX (SCREEN_HEIGHT - BALL_HEIGHT)

int vga_ball_fd;

/* Read and print the background color */
void print_background_color() {
  vga_ball_arg_t vla;
  
  if (ioctl(vga_ball_fd, VGA_BALL_READ_BACKGROUND, &vla)) {
      perror("ioctl(VGA_BALL_READ_BACKGROUND) failed");
      return;
  }
  printf("%02x %02x %02x\n",
	 vla.background.red, vla.background.green, vla.background.blue);
}

/* Set the background color */
void set_background_color(const vga_ball_color_t *c)
{
  vga_ball_arg_t vla;
  vla.background = *c;
  if (ioctl(vga_ball_fd, VGA_BALL_WRITE_BACKGROUND, &vla)) {
      perror("ioctl(VGA_BALL_SET_BACKGROUND) failed");
      return;
  }
}

/* Set the ball coordinates */
void set_ball_coords(uint16_t x, uint16_t y)
{
    vga_ball_arg_t vla;
    memset(&vla, 0, sizeof(vla));
    vla.x = x;
    vla.y = y;

    if (ioctl(vga_ball_fd, VGA_BALL_WRITE_COORDS, &vla)) {
        perror("ioctl(VGA_BALL_WRITE_COORDS) failed");
    }
}

int main()
{
  vga_ball_arg_t vla;
  int x = 340;
  int y = 240;
  int dx = 3;
  int dy = 2;

  static const char filename[] = "/dev/vga_ball";

  printf("VGA ball Userspace program started\n");

  if ( (vga_ball_fd = open(filename, O_RDWR)) == -1) {
    fprintf(stderr, "could not open %s\n", filename);
    return -1;
  }

  vga_ball_color_t bg = {0x00f7, 0x00cd, 0x0072};
  set_background_color(&bg);

  set_ball_coords((uint16_t)x, (uint16_t)y);

  while (1) {
      int next_x = x + dx;
      int next_y = y + dy;

      if (next_x < X_MIN) {
          next_x = X_MIN;
          dx = -dx;
      } else if (next_x > X_MAX) {
          next_x = X_MAX;
          dx = -dx;
      }

      if (next_y < Y_MIN) {
          next_y = Y_MIN;
          dy = -dy;
      } else if (next_y > Y_MAX) {
          next_y = Y_MAX;
          dy = -dy;
      }

      x = next_x;
      y = next_y;

      set_ball_coords((uint16_t)x, (uint16_t)y);

      usleep(16667);
  }

  printf("VGA BALL Userspace program terminating\n");
  return 0;
}
