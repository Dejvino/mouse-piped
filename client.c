#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

int debug = 0; // verbose debug mode
int dummy = 0; // dry-run mode
int mouse_move_fallback = 0;

Display *dpy;
Window root_window;

static void trigger_event(char* command, int param)
{
  if (dummy) return;
  char cmd[64];
  sprintf(cmd, "xdotool %s %d", command, param);
  system(cmd);
}

int mouse_x = 100;
int mouse_y = 100;

static void mouse_move(int x, int y)
{
  if (debug) {
    printf("Mouse -> %d %d\n", x, y);
  }
  if (dummy) return;
  if (mouse_move_fallback) {
    char cmd[64];
    sprintf(cmd, "xdotool mousemove %d %d", x, y);
    system(cmd);
  } else {
    XWarpPointer(dpy, None, root_window, 0, 0, 0, 0, x, y);
    XFlush(dpy);
  }
}

static void mouse_click(int button)
{
  if (debug) printf("Mouse CLICK %d\n", button);
  trigger_event("click", button + 1);
}

static void on_mouse_press(int code, int value)
{
  int button = code - BTN_LEFT + 1;
  if (debug) printf("Mouse PRESS %d\n", button);
  trigger_event("mousedown", button);
}

static void on_mouse_release(int code, int value)
{
  int button = code - BTN_LEFT + 1;
  if (debug) printf("Mouse RELEASE %d\n", button);
  trigger_event("mouseup", button);
}

static void on_mouse_btn(int code, int value)
{
  if (value) {
    on_mouse_press(code, value);
  } else {
    on_mouse_release(code, value);
  }
}

static void on_mouse_abs(int code, int value)
{
  int mouse_moved = 0;
  switch (code) {
    case ABS_X: mouse_x = value; mouse_moved = 1; break;
    case ABS_Y: mouse_y = value; mouse_moved = 1; break;
  }
  if (mouse_moved) {
    mouse_move(mouse_x, mouse_y);
  }
}

static void on_key(int code, int value)
{
  if (value) {
    if (debug) printf("Key DOWN %d\n", value);
    trigger_event("keydown", code);
  } else {
    if (debug) printf("Key UP %d\n", value);
    trigger_event("keyup", code);
  }
}

int main(int argc, char *argv[])
{
  opterr = 0;
  int c;
  while ((c = getopt (argc, argv, "vd")) != -1)
    switch (c)
      {
      case 'v':
        debug = 1;
        break;
      case 'd':
        dummy = 1;
        break;
      default:
        abort ();
      }

  if (debug) printf("Starting.\n");

  if (debug) printf("Opening display...");
  if ((dpy = XOpenDisplay(0)) == NULL) {
    if (debug) printf("Failed.\n");
    else printf("Could not open X display.\n");
    printf("\t...using fallback via xdotool.\n");
    mouse_move_fallback = 1;
  } else {
    if (debug) printf("OK\n", dpy);
    if (debug) printf("Getting root window...\n");
    root_window = XRootWindow(dpy, 0);
    if (debug) printf("Selecting input...\n");
    XSelectInput(dpy, root_window, KeyReleaseMask);
    if (debug) printf("Inited.\n");
  }

  if (debug) printf("Awaiting events.\n");

  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  while ((read = getline(&line, &len, stdin)) != -1) {
    char* type;
    int code;
    int value;
    scanf("%ms %d %d", &type, &code, &value);

    if (debug >= 3) printf("Type: %s, Code: %d, Value: %d\n", type, code, value);

    if (strcmp(type, "abs") == 0) {
      on_mouse_abs(code, value);
    } else if (strcmp(type, "btn") == 0) {
      on_mouse_btn(code, value);
    } else if (strcmp(type, "key") == 0) {
      on_key(code, value);
    }

    free(type);
    fflush(stdout);
  }
  free(line);

  if (debug) printf("Terminating.\n");
  return 0;
}
