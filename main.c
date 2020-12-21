#include "common.h"
#include "image.h"
#include "fonts.h"
#include "files.h"

#include <X11/Xutil.h>

#define ZOOM_AMT 1.1
#define MOVE_AMT 10
#define maprange(s, a1, a2, b1, b2) ((b1) + ((s) - (a1)) * ((b2)-(b1)) / ((a2 - a1)))
#define min(a,b) (((a) > (b)) ? (b) : (a))
#define max(a,b) (((a) < (b)) ? (b) : (a))

Display *display;

void sbimg_error(char *format, ...) {
        va_list ap;
        va_start(ap, format);
        vfprintf(stderr, format, ap);
        va_end(ap);
        exit(EXIT_FAILURE);
}

XImage *sbimg_create_ximage(struct sbimg_image *image, double zoom) {
        /* Xlib has every pixel take up 4 bytes??? idk tbh */
        size_t x, y, xwidth, xheight;
        char *data;
        XImage *ximage;

        xwidth = image->width * zoom;
        xheight = image->height * zoom;
        data = malloc(sizeof(char) * xwidth * xheight * 4);
        ximage = XCreateImage(
                display,
                CopyFromParent,
                24,
                ZPixmap,
                0,
                data,
                xwidth,
                xheight,
                32,
                0
        );

        for (x = 0; x < xwidth; x++) {
                for (y = 0; y < xheight; y++) {
                        size_t ix = maprange(x, 0, xwidth, 0, image->width);
                        size_t iy = maprange(y, 0, xheight, 0, image->height);
                        struct sbimg_pixel p = sbimg_image_get_pixel(image, ix, iy);
                        XPutPixel(ximage, x, y, (p.r << 16) + (p.g << 8) + (p.b << 0));
                }
        }

        return ximage;
}

int main(int argc, char **argv) {
        int window_width, window_height, center_x, center_y, text_height;
        double zoom = 1.0;
        struct sbimg_image image;
        struct sbimg_textbox textbox;
        struct sbimg_files files;
        XImage *ximage;
        GC gc;
        Atom delete_message;
        Window window, image_window, text_window;

        if (argc != 2) {
                sbimg_error("Please pass exactly 1 argument\n");
        }
        sbimg_files_init(&files, argv[1]);

        display = XOpenDisplay(NULL);
        sbimg_image_init(&image, sbimg_files_curr(&files));
        ximage = sbimg_create_ximage(&image, zoom);
        window_width = ximage->width;
        window_height = ximage->height;
        center_x = window_width / 2;
        center_y = window_height / 2;

        window = XCreateSimpleWindow(
                display,
                DefaultRootWindow(display),
                0, 0, /* x, y */
                window_width, window_height,
                0, /* border width (seperate from window manager) */
                WhitePixel(display, DefaultScreen(display)), /* border color */
                WhitePixel(display, DefaultScreen(display)) /* background color */
        );
        XSelectInput(
                display,
                window,
                KeyPressMask | StructureNotifyMask | ExposureMask
        );
        XMapWindow(display, window);
        delete_message = XInternAtom(display, "WM_DELETE_WINDOW", false);
        XSetWMProtocols(display, window, &delete_message, 1);
        gc = XCreateGC(display, window, 0, NULL);

        image_window = XCreateSimpleWindow(
                display,
                window,
                center_x - window_width / 2,
                center_y - window_height / 2,
                window_width, window_height,
                0,
                WhitePixel(display, DefaultScreen(display)), /* border color */
                WhitePixel(display, DefaultScreen(display)) /* background color */
        );
        XMapWindow(display, image_window);

        text_window = XCreateSimpleWindow(
                display,
                window,
                0, 0, 1, 1, 0,
                BlackPixel(display, DefaultScreen(display)),
                BlackPixel(display, DefaultScreen(display))
        );
        sbimg_textbox_init(
                &textbox,
                text_window,
                "Hasklug Nerd Font",
                10.0
        );
        XMapWindow(display, text_window);
        text_height = sbimg_textbox_font_height(&textbox) * 1.5;

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
                        center_x = maprange(
                                center_x,
                                0, window_width,
                                0, e.xconfigure.width
                        );
                        window_width = e.xconfigure.width;

                        center_y = maprange(
                                center_y,
                                0, window_height,
                                0, e.xconfigure.height
                        );
                        window_height = e.xconfigure.height;
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
                                        zoom = 1.0;
                                        center_x = window_width / 2;
                                        center_y = window_height / 2;
                                        sbimg_files_prev(&files);
                                        XDestroyImage(ximage);
                                        sbimg_image_destroy(&image);
                                        sbimg_image_init(
                                                &image,
                                                sbimg_files_curr(&files)
                                        );
                                        ximage = sbimg_create_ximage(&image, zoom);
                                } else {
                                        center_x -= MOVE_AMT;
                                }
                                break;
                        case XK_j:
                                if (e.xkey.state & ShiftMask) {
                                        zoom /= ZOOM_AMT;
                                        XDestroyImage(ximage);
                                        ximage = sbimg_create_ximage(&image, zoom);
                                } else {
                                        center_y += MOVE_AMT;
                                }
                                break;
                        case XK_k:
                                if (e.xkey.state & ShiftMask) {
                                        zoom *= ZOOM_AMT;
                                        XDestroyImage(ximage);
                                        ximage = sbimg_create_ximage(&image, zoom);
                                } else {
                                        center_y -= MOVE_AMT;
                                }
                                break;
                        case XK_l:
                                if (e.xkey.state & ShiftMask) {
                                        zoom = 1.0;
                                        center_x = window_width / 2;
                                        center_y = window_height / 2;
                                        sbimg_files_next(&files);
                                        XDestroyImage(ximage);
                                        sbimg_image_destroy(&image);
                                        sbimg_image_init(
                                                &image,
                                                sbimg_files_curr(&files)
                                        );
                                        ximage = sbimg_create_ximage(&image, zoom);
                                } else {
                                        center_x += MOVE_AMT;
                                }
                                break;
                        }
                        redraw = true;
                }

                if (redraw) {
                        char str[1024] = {0};
                        int top_left_x, top_left_y, im_width, im_height;

                        top_left_x = center_x - ximage->width / 2;
                        top_left_y = center_y - ximage->height / 2;

                        im_width = ximage->width + min(0, top_left_x)
                                + min(0, window_width - top_left_x - ximage->width);
                        im_height = ximage->height + min(0, top_left_y)
                                + min(0, window_height - top_left_y - ximage->height);

                        XMoveResizeWindow(
                                display,
                                image_window,
                                top_left_x,
                                top_left_y,
                                ximage->width,
                                ximage->height
                        );
                        XPutImage(
                                display,
                                image_window,
                                gc,
                                ximage,
                                max(0, -top_left_x),
                                max(0, -top_left_y),
                                max(0, -top_left_x),
                                max(0, -top_left_y),
                                im_width,
                                im_height
                        );

                        XClearWindow(display, text_window);
                        XMoveResizeWindow(
                                display,
                                text_window,
                                0, window_height - text_height,
                                window_width, text_height
                        );
                        strcpy(str, sbimg_files_curr(&files));
                        sbimg_textbox_write(
                                &textbox,
                                text_height, 0,
                                basename(str)
                        );
                        sprintf(
                                str,
                                "[%d/%d]",
                                files.idx + 1,
                                files.file_count
                        );
                        sbimg_textbox_write(
                                &textbox,
                                text_height, window_width,
                                str
                        );
                }
        }

cleanup:
        sbimg_files_destroy(&files);
        XDestroyImage(ximage);
        sbimg_image_destroy(&image);
        sbimg_textbox_destroy(&textbox);
        XFreeGC(display, gc);
        XDestroyWindow(display, image_window);
        XDestroyWindow(display, text_window);
        XDestroyWindow(display, window);
        XCloseDisplay(display);
        return EXIT_SUCCESS;
}

/* 18,612 in use at exit from xft */
/* 214,512 in use at exit from xft */
