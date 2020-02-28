#define VERSION "mp"

int debug = 0; /* verbose debug mode */

#define VERBOSE if (debug) printf
#define VERBOSE_DEBUG if (debug >= 2) printf

void die(char* err)
{
  fprintf(stderr, "ERROR: %s\n", err);
  exit(1);
}

#define TRY(cmd, msg) { if ((cmd) < 0) die(msg); }
