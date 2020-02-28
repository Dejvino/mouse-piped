#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include "common.h"

int dummy = 0; /* dry-run mode */
int move_mode = 0; /* 0 = abs, 1 = rel */

int fd;
struct uinput_user_dev uidev;

void create_device()
{
  VERBOSE("Opening /dev/uinput.\n");
  TRY(fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK), "open /dev/uinput");

  VERBOSE("Setting up KEY keyboard device.\n");
  TRY(ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0, "ioctl EV_KEY");
  for (int i = 0; i < 6; i++) {
      TRY(ioctl(fd, UI_SET_KEYBIT, BTN_LEFT + i) < 0, "ioctl BTN_*");
  }
  for (int i = 0; i < 250; i++) {
      TRY(ioctl(fd, UI_SET_KEYBIT, i) < 0, "ioctl KEY_*");
  }

  /* either ABS or REL is supported at one time */
  if (move_mode == 0) {
    VERBOSE("Setting up ABS pointer device.\n");
    TRY(ioctl(fd, UI_SET_EVBIT, EV_ABS) < 0, "ioctl EV_ABS");
    TRY(ioctl(fd, UI_SET_ABSBIT, ABS_X) < 0, "ioctl ABS_X");
    TRY(ioctl(fd, UI_SET_ABSBIT, ABS_Y) < 0, "ioctl ABS_Y");
  } else {
    VERBOSE("Setting up REL pointer device.\n");
    TRY(ioctl(fd, UI_SET_EVBIT, EV_REL) < 0, "ioctl EV_REL");
    TRY(ioctl(fd, UI_SET_ABSBIT, REL_X) < 0, "ioctl REL_X");
    TRY(ioctl(fd, UI_SET_ABSBIT, REL_Y) < 0, "ioctl REL_Y");
  }

  VERBOSE("Initializing input device.\n");
  memset(&uidev, 0, sizeof(uidev));
  snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "mouse-piped");
  uidev.id.bustype = BUS_USB;
  uidev.id.vendor  = 0x01;
  uidev.id.product = 0x01;
  uidev.id.version = 1;
  /* TODO: real values */
  uidev.absmax[ABS_X]=1000;
  uidev.absmax[ABS_Y]=1000;
  uidev.absmax[ABS_PRESSURE]=100;

  TRY(write(fd, &uidev, sizeof(uidev)) < 0, "write device config");
  TRY(ioctl(fd, UI_DEV_CREATE) < 0, "ioctl DEV_CREATE");

  VERBOSE("Device created. Spooling up.\n");
  sleep(2);
}

void emit_event(int type, int code, int value)
{
  struct input_event ev;
  memset(&ev, 0, sizeof(struct input_event));
  ev.type = type;
  ev.code = code;
  ev.value = value;
  TRY(write(fd, &ev, sizeof(struct input_event)) < 0, "write event");
  VERBOSE("Event: %d %d %d\n", type, code, value);
}

void destory_device()
{
  TRY(ioctl(fd, UI_DEV_DESTROY) < 0, "ioctl DEV_DESTROY");
  close(fd);
}

int main(int argc, char *argv[])
{
  opterr = 0;
  int c;
  while ((c = getopt (argc, argv, "vdrh")) != -1) {
    switch (c)
      {
      case 'v':
        debug = 1;
        break;
      case 'd':
        dummy = 1;
        break;
      case 'r':
        move_mode = 1;
        break;
      case 'h':
      default:
        printf("usage: consumer [OPTIONS]\n");
        printf("\t-v ... verbose output.\n");
        printf("\t-d ... dry-run / dummy, doesn't actually send events to the device.\n");
        printf("\t-r ... relative pointer mode, absolute otherwise.\n");
        return 0;
      }
  }

  VERBOSE("Creating uinput device...\n");
  create_device();
  VERBOSE("OK\n");

  VERBOSE("Awaiting events.\n");

  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  while ((read = getline(&line, &len, stdin)) != -1) {
    char* check;
    int type;
    int code;
    int value;
    scanf("%ms %d %d %d", &check, &type, &code, &value);

    VERBOSE_DEBUG("[%s] Type: %d, Code: %d, Value: %d\n",
      check, type, code, value);

    if (strcmp(check, VERSION) == 0) {
      emit_event(type, code, value);
    }

    free(check);
    fflush(stdout);
  }
  free(line);

  VERBOSE("Terminating.\n");
  return 0;
}
