#ifndef SBIMG_COMMON_H
#define SBIMG_COMMON_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <libgen.h>

#include <X11/Xlib.h>

#define DEBUG_BOOL(b) printf("%s\n", (b) ? "true" : "false")
#define true 1
#define false 0

#define maprange(s, a1, a2, b1, b2) ((b1) + ((s) - (a1)) * ((b2)-(b1)) / ((a2 - a1)))
#define min(a,b) (((a) > (b)) ? (b) : (a))
#define max(a,b) (((a) < (b)) ? (b) : (a))

extern Display *display;
void sbimg_error(char *format, ...);

#endif
