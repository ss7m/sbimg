#include "common.h"
#include "image.h"
#include "fonts.h"
#include "files.h"
#include "window.h"

Display *display;

#define MOVE_TIMEOUT 60000000L

static long timespec_diff(struct timespec *x, struct timespec *y) {
        long xnsec = x->tv_sec * 1e9 + x->tv_nsec;
        long ynsec = y->tv_sec * 1e9 + y->tv_nsec;
        return xnsec - ynsec;
}

void sbimg_error(char *format, ...) {
        va_list ap;
        va_start(ap, format);
        vfprintf(stderr, format, ap);
        va_end(ap);
        exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
        struct sbimg_winstate winstate;
        struct timespec last_move;
        Atom delete_message;
        int numkey_used = false;
        unsigned int count = 1;

        if (argc != 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
                printf(
                        "Usage: sbimg [file]\n"
                        "\n"
                        "A simple X image viewer\n"
                        "\n"
                        "Options:\n"
                        "  -h, --help     print this message and exit\n"
                        "\n"
                        "Controls:\n"
                        "  q              quit program\n"
                        "  h              move image left\n"
                        "  j              move image down\n"
                        "  k              move image up\n"
                        "  l              move image right\n"
                        "  H              go to previous image\n"
                        "  J              zoom out\n"
                        "  K              zoom in\n"
                        "  L              go to next image\n"
                        "  r              rotate image clockwise\n"
                        "  R              rotate image counter-clockwise\n"
                        "\n"
                        "Commands can optionally be prepended with a count\n"
                        "\n"
                );
                exit(EXIT_FAILURE);
        }

        last_move.tv_sec = last_move.tv_nsec = -1;
        display = XOpenDisplay(NULL);

        {
                struct sbimg_files files;
                sbimg_files_init(&files, argv[1]);
                sbimg_winstate_init(
                        &winstate,
                        &files,
                        "Source Code Pro",
                        10.0
                );
        }

        XSelectInput(
                display,
                winstate.window,
                KeyPressMask | StructureNotifyMask | ExposureMask
        );
        delete_message = XInternAtom(display, "WM_DELETE_WINDOW", false);
        XSetWMProtocols(display, winstate.window, &delete_message, 1);
        sbimg_winstate_redraw(&winstate, true);

        for(;;) {
                struct timespec curr_time;
                int force_redraw = false, keysym;
                XEvent e;

                /* discard extra events */
                for (;;) {
                        int discard = false;
                        XEvent ne;
                        XNextEvent(display, &e);
                        clock_gettime(CLOCK_REALTIME, &curr_time);

                        if (XEventsQueued(display, QueuedAlready) == 0) {
                                break;
                        }
                        XPeekEvent(display, &ne);
                        switch(e.type) {
                                case Expose:
                                case ConfigureNotify:
                                        discard = e.type == ne.type;
                                        break;
                                case KeyPress:
                                        discard = (ne.type == KeyPress || ne.type == KeyRelease)
                                                && ne.xkey.keycode == e.xkey.keycode;
                                        break;
                        }
                        if (!discard) {
                                break;
                        }
                }

                switch(e.type) {
                case Expose:
                        force_redraw = true;
                        break;
                case ConfigureNotify:
                        sbimg_winstate_set_dimensions(
                                &winstate,
                                e.xconfigure.width,
                                e.xconfigure.height
                        );
                        break;
                case ClientMessage:
                        if ((Atom)e.xclient.data.l[0] == delete_message) {
                                goto end_program;
                        }
                        break;
                case KeyPress:
                        /* limit movement on held key presses with timeouts */
                        if(timespec_diff(&curr_time, &last_move) < MOVE_TIMEOUT) {
                                break;
                        } else {
                                last_move = curr_time;
                        }

                        keysym = XLookupKeysym(&e.xkey, 0);
                        switch (keysym) {
                        case XK_q:
                                goto end_program;
                        case XK_h:
                                if (e.xkey.state & ShiftMask) {
                                        sbimg_winstate_shift_file(&winstate, -count);
                                } else {
                                        sbimg_winstate_translate(
                                                &winstate, 
                                                -count, 0
                                        );
                                }
                                numkey_used = false;
                                count = 1;
                                break;
                        case XK_j:
                                if (e.xkey.state & ShiftMask) {
                                        sbimg_winstate_zoom(&winstate, -count);
                                } else {
                                        sbimg_winstate_translate(
                                                &winstate,
                                                0, count
                                        );
                                }
                                numkey_used = false;
                                count = 1;
                                break;
                        case XK_k:
                                if (e.xkey.state & ShiftMask) {
                                        sbimg_winstate_zoom(&winstate, count);
                                } else {
                                        sbimg_winstate_translate(
                                                &winstate,
                                                0, -count
                                        );
                                }
                                numkey_used = false;
                                count = 1;
                                break;
                        case XK_l:
                                if (e.xkey.state & ShiftMask) {
                                        sbimg_winstate_shift_file(&winstate, count);
                                } else {
                                        sbimg_winstate_translate(
                                                &winstate,
                                                count, 0
                                        );
                                }
                                numkey_used = false;
                                count = 1;
                                break;
                        case XK_r:
                                if (e.xkey.state & ShiftMask) {
                                        sbimg_winstate_rotate(&winstate, -count);
                                } else {
                                        sbimg_winstate_rotate(&winstate, count);
                                }
                                numkey_used = false;
                                count = 1;
                                break;
                        case XK_0:
                        case XK_1:
                        case XK_2:
                        case XK_3:
                        case XK_4:
                        case XK_5:
                        case XK_6:
                        case XK_7:
                        case XK_8:
                        case XK_9:
                                if (numkey_used) {
                                        count = count * 10 + keysym - XK_0;
                                } else {
                                        count = keysym - XK_0;
                                }
                                numkey_used = true;
                                break;
                        }
                }

                sbimg_winstate_redraw(&winstate, force_redraw);
        }

end_program:
        printf("%s\n", winstate.files.files[winstate.files.idx]);

        sbimg_winstate_destroy(&winstate);
        XCloseDisplay(display);
        return EXIT_SUCCESS;
}

/* 18,612 in use at exit from xft */
/* 214,512 in use at exit from xft */
