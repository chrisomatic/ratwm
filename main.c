#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "log.c"
#include "draw.c"
#include "wm.c"

void main()
{
    wm_create();
    wm_run();
    wm_destroy();
}
