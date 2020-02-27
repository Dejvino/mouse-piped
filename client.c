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

int debug = 1;
int dummy = 1;

Display *dpy;
Window root_window;

int mouse_x = 100;
int mouse_y = 100;

static void mouse_move(int x, int y)
{
  if (debug) {
    printf("Mouse -> %d %d\n", x, y);
  }
  if (dummy) return;
  XWarpPointer(dpy, None, root_window, 0, 0, 0, 0, x, y);
  XFlush(dpy);
}

static void mouse_click(int button)
{
  if (debug) {
    printf("Mouse CLICK %d\n", button);
  }
  if (dummy) return;
  char cmd[64];
  sprintf(cmd, "xdotool click %d", button);
  system(cmd);
}

static void on_mouse_btn(int code, int value)
{
  switch (code) {
    case BTN_LEFT: mouse_click(1); break;
    case BTN_MIDDLE: mouse_click(2); break;
    case BTN_RIGHT: mouse_click(3); break;
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

int main(int argc, char *argv[])
{
  if (debug) printf("Starting.\n");

  if (debug) printf("Opening display...");
  if ((dpy = XOpenDisplay(0)) == NULL) {
    if (debug) printf("Failed.\n");
    else printf("Could not open X display.\n");
    exit(1);
  }
  if (debug) printf("OK\n", dpy);
  if (debug) printf("Getting root window...\n");
  root_window = XRootWindow(dpy, 0);
  if (debug) printf("Selecting input...\n");
  XSelectInput(dpy, root_window, KeyReleaseMask);
  if (debug) printf("Inited.\n");

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
    }

    free(type);
  }
  free(line);

  if (debug) printf("Terminating.\n");
  return 0;
}
