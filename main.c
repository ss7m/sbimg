#include "common.h"
#include "image.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define MOVE_AMT 10

Display *display;

void sbimg_error(char *format, ...) {
        va_list ap;
        va_start(ap, format);
        vfprintf(stderr, format, ap);
        va_end(ap);
        exit(EXIT_FAILURE);
}

XImage *sbimg_create_ximage(struct sbimg_image *image) {
        /* Xlib has every pixel take up 4 bytes??? idk tbh */
        size_t x, y;
        char *data = malloc(sizeof(char) * image->width * image->height * 4);
        XImage *ximage = XCreateImage(
                display,
                CopyFromParent,
                24,
                ZPixmap,
                0,
                data,
                image->width,
                image->height,
                32,
                0
        );

        for (x = 0; x < image->width; x++) {
                for (y = 0; y < image->height; y++) {
                        struct sbimg_pixel p = sbimg_image_get_pixel(image, x, y);
                        XPutPixel(ximage, x, y, (p.r << 16) + (p.g << 8) + (p.b << 0));
                }
        }

        return ximage;
}

int main(void) {
        int width, height, center_x, center_y;
        struct sbimg_image image;
        XImage *ximage;
        GC gc;
        Window window, child_window;

        display = XOpenDisplay(NULL);
        sbimg_image_init(&image, "/home/sam-barr/Pictures/bliss.png");
        ximage = sbimg_create_ximage(&image);
        width = image.width;
        height = image.height;
        center_x = width / 2;
        center_y = height / 2;

        window = XCreateSimpleWindow(
                display,
                DefaultRootWindow(display),
                0, 0, /* x, y */
                width, height, /* width, height */
                0, /* border width (seperate from window manager) */
                WhitePixel(display, DefaultScreen(display)), /* border color */
                WhitePixel(display, DefaultScreen(display)) /* background color */
        );
        child_window = XCreateSimpleWindow(
                display,
                window,
                center_x - width / 2,
                center_y - height / 2,
                width, height,
                500,
                WhitePixel(display, DefaultScreen(display)), /* border color */
                WhitePixel(display, DefaultScreen(display)) /* background color */
        );
        XSelectInput(
                display,
                window,
                KeyPressMask | StructureNotifyMask | ExposureMask
        );
        XMapWindow(display, window);
        XMapWindow(display, child_window);
        gc = XCreateGC(display, window, 0, NULL);

        XPutImage(
                display,
                child_window,
                gc,
                ximage,
                0, 0, 0, 0,
                image.width, image.height
        );

        for(;;) {
                int redraw = false;
                double percent;
                XEvent e;

                XNextEvent(display, &e);

                switch(e.type) {
                case Expose:
                        redraw = true;
                        break;
                case ConfigureNotify:
                        percent = ((1.0) * center_x) / width;
                        center_x = percent * e.xconfigure.width;
                        width = e.xconfigure.width;

                        percent = ((1.0) * center_y) / height;
                        center_y = percent * e.xconfigure.height;
                        height = e.xconfigure.height;
                        redraw = true;

                        break;
                case KeyPress:
                        switch (XLookupKeysym(&e.xkey, 0)) {
                        case XK_q:
                                goto cleanup;
                        case XK_h:
                                center_x -= MOVE_AMT;
                                break;
                        case XK_j:
                                center_y += MOVE_AMT;
                                break;
                        case XK_k:
                                center_y -= MOVE_AMT;
                                break;
                        case XK_l:
                                center_x += MOVE_AMT;
                                break;
                        }
                        redraw = true;
                }

                if (redraw) {
                        XMoveResizeWindow(
                                display,
                                child_window,
                                center_x - image.width / 2,
                                center_y - image.height / 2,
                                image.width,
                                image.height
                        );
                        XPutImage(
                                display,
                                child_window,
                                gc,
                                ximage,
                                0, 0, 0, 0,
                                image.width,
                                image.height
                        );
                }
        }

cleanup:
        XDestroyImage(ximage);
        sbimg_image_destroy(&image);
        XFreeGC(display, gc);
        XDestroyWindow(display, child_window);
        XDestroyWindow(display, window);
        XCloseDisplay(display);
        return EXIT_SUCCESS;
}

/* 18,612 in use at exit from xft */
