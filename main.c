#include "common.h"
#include "image.h"
#include "fonts.h"
#include "files.h"
#include "window.h"

#define ZOOM_AMT 1.1
#define MOVE_AMT 10

Display *display;

void sbimg_error(char *format, ...) {
        va_list ap;
        va_start(ap, format);
        vfprintf(stderr, format, ap);
        va_end(ap);
        exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
        struct sbimg_winstate winstate;
        Atom delete_message;

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

        for(;;) {
                int redraw = false;
                XEvent e;

                /* discard extra events */
                for (;;) {
                        int discard = false;
                        XEvent ne;
                        XNextEvent(display, &e);
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
                        redraw = true;
                        break;
                case ConfigureNotify:
                        sbimg_winstate_set_dimensions(
                                &winstate,
                                e.xconfigure.width,
                                e.xconfigure.height
                        );
                        redraw = true;
                        break;
                case ClientMessage:
                        if ((Atom)e.xclient.data.l[0] == delete_message) {
                                goto cleanup;
                        }
                        break;
                case KeyPress:
                        switch (XLookupKeysym(&e.xkey, 0)) {
                        case XK_q:
                                goto cleanup;
                        case XK_h:
                                if (e.xkey.state & ShiftMask) {
                                        sbimg_winstate_prev_image(&winstate);
                                } else {
                                        sbimg_winstate_translate(
                                                &winstate, 
                                                -MOVE_AMT,
                                                0
                                        );
                                }
                                break;
                        case XK_j:
                                if (e.xkey.state & ShiftMask) {
                                        sbimg_winstate_zoom(&winstate, 1/ZOOM_AMT);
                                } else {
                                        sbimg_winstate_translate(
                                                &winstate,
                                                0,
                                                MOVE_AMT
                                        );
                                }
                                break;
                        case XK_k:
                                if (e.xkey.state & ShiftMask) {
                                        sbimg_winstate_zoom(&winstate, ZOOM_AMT);
                                } else {
                                        sbimg_winstate_translate(
                                                &winstate,
                                                0,
                                                -MOVE_AMT
                                        );
                                }
                                break;
                        case XK_l:
                                if (e.xkey.state & ShiftMask) {
                                        sbimg_winstate_next_image(&winstate);
                                } else {
                                        sbimg_winstate_translate(
                                                &winstate,
                                                MOVE_AMT,
                                                0
                                        );
                                }
                                break;
                        }
                        redraw = true;
                }

                if (redraw) {
                        sbimg_winstate_redraw(&winstate);
                }
        }

cleanup:
        sbimg_winstate_destroy(&winstate);
        XCloseDisplay(display);
        return EXIT_SUCCESS;
}

/* 18,612 in use at exit from xft */
/* 214,512 in use at exit from xft */
