#ifndef SBIMG_COMMON_H
#define SBIMG_COMMON_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#define DEBUG_BOOL(b) printf("%s\n", (b) ? "true" : "false")
#define true 1
#define false 0

void sbimg_error(char *format, ...);

#endif
