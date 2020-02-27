#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XInput2.h>

/** Release Key Combo Test
 * MSC_SCAN (?)
keyboard event: type 4 code 4 value 219
 * KEY_LEFTMETA 1
keyboard event: type 1 code 125 value 1
 * SYNC (?)
keyboard event: type 0 code 0 value 0
keyboard event: type 4 code 4 value 219
 * KEY_LEFTMETA 0
keyboard event: type 1 code 125 value 0
keyboard event: type 0 code 0 value 0
keyboard event: type 4 code 4 value 219
 * KEY_LEFTMETA 1
keyboard event: type 1 code 125 value 1
keyboard event: type 0 code 0 value 0
keyboard event: type 4 code 4 value 45
 * KEY_X 1
keyboard event: type 1 code 45 value 1
keyboard event: type 0 code 0 value 0
keyboard event: type 4 code 4 value 45
 * KEY_X 0
keyboard event: type 1 code 45 value 0
keyboard event: type 0 code 0 value 0
keyboard event: type 4 code 4 value 219
 * KEY_LEFTMETA 0
keyboard event: type 1 code 125 value 0
keyboard event: type 0 code 0 value 0
*/

static long long timestamp()
{
    struct timeval te; 
    gettimeofday(&te, NULL);
    long long ms = te.tv_sec * 1000LL + te.tv_usec / 1000LL;
    return ms;
}

char key_pressed[256];
int abs_changed = 0;
int abs_values[256];
int abs_values_changed[256];

static void on_key_event(int code, int value)
{
	printf("key %d %d\n", code, value);
	key_pressed[code] = value;
}

static void on_mouse_abs_event(int code, int value)
{
	abs_changed = 1;
  abs_values[code] = value;
  abs_values_changed[code] = 1;
}

static void on_mouse_btn_event(int code, int value)
{
	printf("btn %d %d\n", code, value);
}

static void on_mouse_rel_event(int code, int value)
{
	printf("rel %d %d\n", code, value);
}

static int get_release_pressed()
{
	// TODO: configurable release
	return (key_pressed[KEY_LEFTMETA] > 0
		&& key_pressed[KEY_X] > 0);
}

int main(int argc, char* argv[])
{ 
  memset(key_pressed, 0, sizeof(key_pressed));

  // TODO: read event files from argv
  char* keyboard_file = "/dev/input/event1";
  char* mouse_file = "/dev/input/event4";

  printf("Press LMETA + X to release grab.\n");

  sleep(1);
  int rcode = 0;

  char keyboard_name[256] = "Unknown";
  int keyboard_fd = open(keyboard_file, O_RDONLY | O_NONBLOCK);
  if ( keyboard_fd == -1 ) {
    printf("Failed to open keyboard.\n");
    exit(1);
  }
  rcode = ioctl(keyboard_fd, EVIOCGNAME(sizeof(keyboard_name)), keyboard_name);
  printf("Reading From : %s \n", keyboard_name);

  printf("Getting exclusive access: ");
  rcode = ioctl(keyboard_fd, EVIOCGRAB, 1);
  printf("%s\n", (rcode == 0) ? "SUCCESS" : "FAILURE");
  struct input_event keyboard_event;
  
  char mouse_name[256] = "Unknown";
  int mouse_fd = open(mouse_file, O_RDONLY | O_NONBLOCK);
  if ( mouse_fd == -1 ) {
    printf("Failed to open mouse.\n");
    exit(1);
  }
  rcode = ioctl(mouse_fd, EVIOCGNAME(sizeof(mouse_name)), mouse_name);
  printf("Reading From : %s \n", mouse_name);

  printf("Getting exclusive access: ");
  rcode = ioctl(mouse_fd, EVIOCGRAB, 1);
  printf("%s\n", (rcode == 0) ? "SUCCESS" : "FAILURE");
  struct input_event mouse_event;

  int end = time(NULL) + 60;
  long long next_update = timestamp() + 100LL;
  while ( time(NULL) < end ) {
    if ( read(keyboard_fd, &keyboard_event, sizeof(keyboard_event)) != -1 ) {
      //printf("keyboard event: type %d code %d value %d  \n", keyboard_event.type, keyboard_event.code, keyboard_event.value);
      if (keyboard_event.type == EV_KEY) {
	      on_key_event(keyboard_event.code, keyboard_event.value);
      }
    }
    
    int sz;
    if (sz = read(mouse_fd, &mouse_event, sizeof(mouse_event)) ) {
      if(sz != -1) {
        //printf("mouse event: type %d code %d value %d  \n", mouse_event.type, mouse_event.code, mouse_event.value);
        if (mouse_event.type == EV_ABS) {
          on_mouse_abs_event(mouse_event.code, mouse_event.value);
        } else if (mouse_event.type == EV_REL) {
          on_mouse_rel_event(mouse_event.code, mouse_event.value);
        } else if (mouse_event.type == EV_KEY) {
          on_mouse_btn_event(mouse_event.code, mouse_event.value);
        }
      }
    }

    if (get_release_pressed()) {
	    printf("Release requested.\n");
	    break;
    }
    if ((next_update <= timestamp()) && abs_changed) {
      abs_changed = 0;
      next_update = timestamp() + 100LL;
      for (int code = 0; code < 256; code++) {
        if (abs_values_changed[code]) {
          abs_values_changed[code] = 0;
          printf("abs %d %d\n", code, abs_values[code]);
        }
      }
    }
    fflush(stdout);
  }

  printf("Exiting.\n");
  rcode = ioctl(keyboard_fd, EVIOCGRAB, 1);
  close(keyboard_fd);
  rcode = ioctl(mouse_fd, EVIOCGRAB, 1);
  close(mouse_fd);
  return 0;
}

