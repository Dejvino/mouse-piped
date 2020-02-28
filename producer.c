/* Source: https://gist.github.com/matthewaveryusa/a721aad80ae89a5c69f7c964fa20fec1 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include "common.h"

static long long timestamp()
{
    struct timeval te; 
    gettimeofday(&te, NULL);
    long long ms = te.tv_sec * 1000LL + te.tv_usec / 1000LL;
    return ms;
}

int last_event_syn = 0;

void emit_event(int type, int code, int value)
{
  if (type == EV_SYN && last_event_syn == 1) {
    // dropping
  } else {
    printf("%s %d %d %d\n", VERSION, type, code, value);
    last_event_syn = type == EV_SYN;
  }
}

void emit_event_struct(struct input_event event)
{
  emit_event(event.type, event.code, event.value);
}

#define KEYS_COUNT 256
char key_pressed[KEYS_COUNT];

#define ABS_COUNT 32
int abs_values[ABS_COUNT];
int abs_values_changed[ABS_COUNT];
int abs_changed = 0;

static void on_key_event(int code, int value)
{
	key_pressed[code] = value;
}

static void on_mouse_abs_event(int code, int value)
{
	abs_changed = 1;
  abs_values[code] = value;
  abs_values_changed[code] = 1;
}

static int get_release_pressed()
{
	// TODO: configurable release combination
	return (key_pressed[KEY_LEFTMETA] > 0
		&& key_pressed[KEY_X] > 0);
}

static int send_release_events()
{
	for (int code = 0; code < KEYS_COUNT; code++) {
    if (key_pressed[code] > 0) {
      emit_event(EV_KEY, code, 0);
    }
  }
  emit_event(EV_SYN, 0, 0);
}

int main(int argc, char* argv[])
{ 
  opterr = 0;
  int c;
  while ((c = getopt (argc, argv, "vh")) != -1) {
    switch (c)
      {
      case 'v':
        debug = 1;
        break;
      case 'h':
      default:
        printf("usage: producer [OPTIONS] input_dev\n");
        printf("\t-v ... verbose output.\n");
        printf("\tinput_dev ... input device file path.\n");
        return 0;
      }
  }
  if (optind + 1 != argc) {
    die("expecting exactly one input_dev. Run with -h for details.");
  }
  char* input_file = argv[argc-1];

  memset(key_pressed, 0, sizeof(key_pressed));

  // TODO: read event files from argv
  char* keyboard_file = "/dev/input/event1";
  char* mouse_file = "/dev/input/event4";

  printf("Press LMETA + X to release grab.\n");

  sleep(1);
  int rcode = 0;

  char keyboard_name[256] = "Unknown";
  int keyboard_fd = open(keyboard_file, O_RDONLY | O_NONBLOCK);
  TRY(keyboard_fd, "Failed to open keyboard.");

  rcode = ioctl(keyboard_fd, EVIOCGNAME(sizeof(keyboard_name)), keyboard_name);
  VERBOSE("Reading From : %s \n", keyboard_name);

  VERBOSE("Getting exclusive access: ");
  rcode = ioctl(keyboard_fd, EVIOCGRAB, 1);
  VERBOSE("%s\n", (rcode == 0) ? "SUCCESS" : "FAILURE");
  struct input_event keyboard_event;
  
  char mouse_name[256] = "Unknown";
  int mouse_fd = open(mouse_file, O_RDONLY | O_NONBLOCK);
  TRY(mouse_fd, "Failed to open mouse.\n");
  rcode = ioctl(mouse_fd, EVIOCGNAME(sizeof(mouse_name)), mouse_name);
  VERBOSE("Reading From : %s \n", mouse_name);

  VERBOSE("Getting exclusive access: ");
  rcode = ioctl(mouse_fd, EVIOCGRAB, 1);
  VERBOSE("%s\n", (rcode == 0) ? "SUCCESS" : "FAILURE");
  struct input_event mouse_event;

  int end = time(NULL) + 60;
  long long next_update = timestamp() + 100LL;
  while (time(NULL) < end) {
    if (read(keyboard_fd, &keyboard_event, sizeof(keyboard_event)) != -1) {
      if (keyboard_event.type == EV_KEY) {
	      on_key_event(keyboard_event.code, keyboard_event.value);
      }
      emit_event_struct(keyboard_event);
    }
    
    int sz;
    if (sz = read(mouse_fd, &mouse_event, sizeof(mouse_event))) {
      if(sz != -1) {
        // aggregate ABS mouse events to decrease traffic
        if (mouse_event.type == EV_ABS) {
          on_mouse_abs_event(mouse_event.code, mouse_event.value);
        } else {
          emit_event_struct(mouse_event);
        }
      }
    }

    if (get_release_pressed()) {
	    VERBOSE("Release requested.\n");
      send_release_events();
	    break;
    }

    // send ABS mouse events aggregated
    if ((next_update <= timestamp()) && abs_changed) {
      abs_changed = 0;
      next_update = timestamp() + 100LL;
      for (int code = 0; code < 256; code++) {
        if (abs_values_changed[code]) {
          abs_values_changed[code] = 0;
          emit_event(EV_ABS, code, abs_values[code]);
        }
      }
      emit_event(EV_SYN, 0, 0);
    }
    fflush(stdout);
  }

  VERBOSE("Exiting.\n");
  rcode = ioctl(keyboard_fd, EVIOCGRAB, 1);
  close(keyboard_fd);
  rcode = ioctl(mouse_fd, EVIOCGRAB, 1);
  close(mouse_fd);
  return 0;
}
