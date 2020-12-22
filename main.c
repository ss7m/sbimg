#include "common.h"
#include "image.h"
#include "fonts.h"
#include "files.h"
#include "window.h"

Display *display;

#define MOVE_TIMEOUT 100000000L

long timespec_diff(struct timespec *x, struct timespec *y) {
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

        last_move.tv_sec = last_move.tv_nsec = -1;

        display = XOpenDisplay(NULL);
        {
                struct sbimg_files files;
                if (argc != 2) {
                        sbimg_error("Please pass exactly 1 argument\n");
                }
                sbimg_files_init(&files, argv[1]);
                sbimg_winstate_init(
                        &winstate,
                        &files,
                        "Hasklug Nerd Font",
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
                int force_redraw = false;
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
                                goto cleanup;
                        }
                        break;
                case KeyPress:
                        if(timespec_diff(&curr_time, &last_move) < MOVE_TIMEOUT) {
                                break;
                        } else {
                                last_move = curr_time;
                        }
                        switch (XLookupKeysym(&e.xkey, 0)) {
                        case XK_q:
                                goto cleanup;
                        case XK_h:
                                if (e.xkey.state & ShiftMask) {
                                        sbimg_winstate_prev_image(&winstate);
                                } else {
                                        sbimg_winstate_translate(
                                                &winstate, 
                                                -1,
                                                0
                                        );
                                }
                                break;
                        case XK_j:
                                if (e.xkey.state & ShiftMask) {
                                        sbimg_winstate_zoom(&winstate, -1);
                                } else {
                                        sbimg_winstate_translate(
                                                &winstate,
                                                0,
                                                1
                                        );
                                }
                                break;
                        case XK_k:
                                if (e.xkey.state & ShiftMask) {
                                        sbimg_winstate_zoom(&winstate, 1);
                                } else {
                                        sbimg_winstate_translate(
                                                &winstate,
                                                0,
                                                -1
                                        );
                                }
                                break;
                        case XK_l:
                                if (e.xkey.state & ShiftMask) {
                                        sbimg_winstate_next_image(&winstate);
                                } else {
                                        sbimg_winstate_translate(
                                                &winstate,
                                                1,
                                                0
                                        );
                                }
                                break;
                        }
                }

                sbimg_winstate_redraw(&winstate, force_redraw);
        }

cleanup:
        sbimg_winstate_destroy(&winstate);
        XCloseDisplay(display);
        return EXIT_SUCCESS;
}

/* 18,612 in use at exit from xft */
/* 214,512 in use at exit from xft */
