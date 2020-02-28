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

static int time_limit = 30; /* set 0 for endless mode */
static long long aggregation_step = 100LL; /* how often to send aggregated updates (in ms) */
static int input_count = 0;

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

static void on_abs_event(int code, int value)
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
  while ((c = getopt (argc, argv, "veh")) != -1) {
    switch (c)
      {
      case 'v':
        debug = 1;
        break;
      case 'e':
        time_limit = 0;
        break;
      case 'h':
      default:
        printf("usage: producer [OPTIONS] input_dev [input_dev ...]\n");
        printf("\t-v ... verbose output.\n");
        printf("\t-e ... endless mode. For production usage.\n");
        printf("\tinput_dev ... input device file path.\n");
        return 0;
      }
  }
  if (optind == argc) {
    die("expecting at least one input_dev parameter. Run with -h for details.");
  }
  input_count = argc - optind;
  char** input_files = &argv[optind];
  int input_fds[input_count];

  printf("Press LMETA + X to release grab.\n");

  /* wait for keys / buttons to be released before starting.
   * TODO: ignore releasing of buttons we don't know are pressed.
   */
  sleep(1);

  for (int i = 0; i < input_count; i++) {
    char* input_file = input_files[i];
    int rcode = 0;

    VERBOSE("Opening input device %s.\n", input_file);
    int input_fd = open(input_file, O_RDONLY | O_NONBLOCK);
    TRY(input_fd, "Failed to open input file.");
    input_fds[i] = input_fd;

    VERBOSE("Getting input device name.\n");
    char input_name[256] = "Unknown";
    TRY(rcode = ioctl(input_fd, EVIOCGNAME(sizeof(input_name)), input_name),
      "ioctl: EVIOCGNAME")
    printf("Reading From: %s \n", input_name);

    VERBOSE("Getting exclusive access.\n");
    TRY(rcode = ioctl(input_fd, EVIOCGRAB, 1), "ioctl: EVIOCGRAB");
  }

  memset(key_pressed, 0, sizeof(key_pressed));
  struct input_event input_event;
  
  if (time_limit > 0) printf("Time limit set to %d seconds.\n", time_limit);
  VERBOSE("Producing events...\n");
  int end = time(NULL) + time_limit;
  long long next_update = timestamp() + 100LL;
  while (time_limit == 0 || time(NULL) < end) {
    for (int i = 0; i < input_count; i++) {
      int input_fd = input_fds[i];
      if (read(input_fd, &input_event, sizeof(input_event)) != -1) {
        if (input_event.type == EV_KEY) {
          on_key_event(input_event.code, input_event.value);
        }
        if (input_event.type == EV_ABS) {
          // aggregate ABS events to decrease traffic
          on_abs_event(input_event.code, input_event.value);
        } else {
          emit_event_struct(input_event);
        }
      }
    }

    if (get_release_pressed()) {
	    VERBOSE("Release requested.\n");
      send_release_events();
	    break;
    }

    // send ABS events aggregated
    if ((next_update <= timestamp()) && abs_changed) {
      abs_changed = 0;
      next_update = timestamp() + aggregation_step;
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
  for (int i = 0; i < input_count; i++) {
    ioctl(input_fds[i], EVIOCGRAB, 1);
    close(input_fds[i]);
  }
  return 0;
}
