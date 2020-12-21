#include "common.h"
#include "image.h"
#include "fonts.h"
#include "files.h"
#include "window.h"

#define tlx(w) ((w)->center_x - (w)->ximage->width / 2)
#define tly(w) ((w)->center_y - (w)->ximage->height / 2)
#define imw(w) ((w)->ximage->width \
        + min(0, tlx(w)) \
        + min(0, (w)->window_width - tlx(w) - (w)->ximage->width))
#define imh(w) ((w)->ximage->height \
        + min(0, tly(w)) \
        + min(0, (w)->window_height - tly(w) - (w)->ximage->height))
#define txth(w) (sbimg_textbox_font_height(&w->textbox) * 1.5)

enum {
        TEXT = (1 << 0),
        IMAGE = (1 << 2)
};

static void sbimg_winstate_gen_ximage(struct sbimg_winstate *winstate) {
        /* Xlib has every pixel take up 4 bytes??? idk tbh */
        size_t x, y, xwidth, xheight;
        char *data;

        xwidth = winstate->image.width * winstate->zoom;
        xheight = winstate->image.height * winstate->zoom;
        data = malloc(sizeof(char) * xwidth * xheight * 4);
        winstate->ximage = XCreateImage(
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
                        size_t ix = maprange(
                                x, 
                                0, xwidth,
                                0, winstate->image.width
                        );
                        size_t iy = maprange(
                                y,
                                0, xheight,
                                0, winstate->image.height
                        );
                        struct sbimg_pixel p = sbimg_image_get_pixel(
                                &winstate->image,
                                ix, iy
                        );
                        XPutPixel(
                                winstate->ximage,
                                x, y,
                                (p.r << 16) + (p.g << 8) + (p.b << 0)
                        );
                }
        }
}

void sbimg_winstate_destroy(struct sbimg_winstate *winstate) {
        sbimg_image_destroy(&winstate->image);
        sbimg_textbox_destroy(&winstate->textbox);
        sbimg_files_destroy(&winstate->files);
        XDestroyImage(winstate->ximage);
        XFreeGC(display, winstate->gc);
        XFreePixmap(display, winstate->pixmap);
        XDestroyWindow(display, winstate->image_window);
        XDestroyWindow(display, winstate->text_window);
        XDestroyWindow(display, winstate->window);
}

void sbimg_winstate_init(
        struct sbimg_winstate *winstate,
        struct sbimg_files *files,
        const char *font_string,
        double font_size)
{
        winstate->files = *files;
        winstate->changes = 0;
        winstate->zoom = 1.0;
        sbimg_image_init(&winstate->image, sbimg_files_curr(files));
        sbimg_winstate_gen_ximage(winstate);
        winstate->window_width = winstate->ximage->width;
        winstate->window_height = winstate->ximage->height;
        winstate->center_x = winstate->ximage->width / 2;
        winstate->center_y = winstate->ximage->height / 2;

        winstate->window = XCreateSimpleWindow(
                display,
                DefaultRootWindow(display),
                0, 0,
                winstate->ximage->width, winstate->ximage->height,
                0,
                WhitePixel(display, DefaultScreen(display)),
                WhitePixel(display, DefaultScreen(display))
        );
        XMapWindow(display, winstate->window);
        winstate->gc = XCreateGC(display, winstate->window, 0, NULL);

        winstate->image_window = XCreateSimpleWindow(
                display,
                winstate->window,
                1, 1, 1, 1, 0, /* dummy default values */
                WhitePixel(display, DefaultScreen(display)),
                WhitePixel(display, DefaultScreen(display))
        );
        XMapWindow(display, winstate->image_window);
        winstate->pixmap = XCreatePixmap(
                display,
                winstate->image_window,
                1, 1, /* default dummy values */
                DefaultDepth(display, DefaultScreen(display))
        );

        winstate->text_window = XCreateSimpleWindow(
                display,
                winstate->window,
                0, 0, 1, 1, 0, /* dummy default values */
                BlackPixel(display, DefaultScreen(display)),
                BlackPixel(display, DefaultScreen(display))
        );
        XMapWindow(display, winstate->text_window);
        sbimg_textbox_init(
                &winstate->textbox,
                winstate->text_window,
                font_string,
                font_size
        );
}

void sbimg_winstate_set_dimensions(
        struct sbimg_winstate *winstate,
        int width,
        int height)
{
        winstate->changes |= TEXT;
        winstate->changes |= IMAGE;
        winstate->center_x = maprange(
                winstate->center_x,
                0, winstate->window_width,
                0, width
        );
        winstate->window_width = width;

        winstate->center_y = maprange(
                winstate->center_y,
                0, winstate->window_height,
                0, height
        );
        winstate->window_height = height;
}

void sbimg_winstate_prev_image(struct sbimg_winstate *winstate) {
        winstate->changes |= IMAGE;
        winstate->changes |= TEXT;
        winstate->zoom = 1.0;
        winstate->center_x = winstate->window_width / 2;
        winstate->center_y = winstate->window_height / 2;
        sbimg_files_prev(&winstate->files);
        XDestroyImage(winstate->ximage);
        sbimg_image_destroy(&winstate->image);
        sbimg_image_init(
                &winstate->image,
                sbimg_files_curr(&winstate->files)
        );
        sbimg_winstate_gen_ximage(winstate);
}

void sbimg_winstate_next_image(struct sbimg_winstate *winstate) {
        winstate->changes |= IMAGE;
        winstate->changes |= TEXT;
        winstate->zoom = 1.0;
        winstate->center_x = winstate->window_width / 2;
        winstate->center_y = winstate->window_height / 2;
        sbimg_files_next(&winstate->files);
        XDestroyImage(winstate->ximage);
        sbimg_image_destroy(&winstate->image);
        sbimg_image_init(
                &winstate->image,
                sbimg_files_curr(&winstate->files)
        );
        sbimg_winstate_gen_ximage(winstate);
}

void sbimg_winstate_translate(struct sbimg_winstate *winstate, int x, int y) {
        winstate->changes |= IMAGE;
        winstate->center_x += x;
        winstate->center_y += y;
}

void sbimg_winstate_zoom(struct sbimg_winstate *winstate, double zoom_amt) {
        winstate->changes |= IMAGE;
        winstate->zoom *= zoom_amt;
        XDestroyImage(winstate->ximage);
        sbimg_winstate_gen_ximage(winstate);
}

void sbimg_winstate_redraw(struct sbimg_winstate *winstate, int force_redraw) {
        char str[1024] = {0};
        int top_left_x, top_left_y, im_width, im_height, text_height;

        top_left_x = tlx(winstate);
        top_left_y = tly(winstate);
        im_width = imw(winstate);
        im_height = imh(winstate);
        text_height = txth(winstate);

        if (winstate->changes & IMAGE || force_redraw) {
                XFreePixmap(display, winstate->pixmap);
                XMoveResizeWindow(
                        display,
                        winstate->image_window,
                        top_left_x,
                        top_left_y,
                        winstate->ximage->width,
                        winstate->ximage->height
                );
                winstate->pixmap = XCreatePixmap(
                        display,
                        winstate->image_window,
                        winstate->ximage->width,
                        winstate->ximage->height,
                        DefaultDepth(display, DefaultScreen(display))
                );
                XPutImage(
                        display,
                        winstate->pixmap,
                        winstate->gc,
                        winstate->ximage,
                        max(0, -top_left_x),
                        max(0, -top_left_y),
                        max(0, -top_left_x),
                        max(0, -top_left_y),
                        im_width,
                        im_height
                );
                XCopyArea(
                        display,
                        winstate->pixmap,
                        winstate->image_window,
                        winstate->gc,
                        0, 0,
                        winstate->ximage->width,
                        winstate->ximage->height,
                        0, 0
                );
        }

        if (winstate->changes & TEXT || force_redraw) {
                XMoveResizeWindow(
                        display,
                        winstate->text_window,
                        0, winstate->window_height - text_height,
                        winstate->window_width, text_height
                );
                XClearWindow(display, winstate->text_window);
                strcpy(str, sbimg_files_curr(&winstate->files));
                sbimg_textbox_write(
                        &winstate->textbox,
                        text_height, 0,
                        basename(str)
                );
                sprintf(
                        str,
                        "[%d/%d]",
                        winstate->files.idx + 1,
                        winstate->files.file_count
                );
                sbimg_textbox_write(
                        &winstate->textbox,
                        text_height, winstate->window_width,
                        str
                );
        }

        winstate->changes = 0;
}
