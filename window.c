#include "common.h"
#include "image.h"
#include "fonts.h"
#include "files.h"
#include "window.h"

#include <math.h>

#define INITIAL_WINDOW_SIZE 500

#define tlx(w) ((w)->center_x - (w)->zoom * (w)->ximage->width / 2)
#define tly(w) ((w)->center_y - (w)->zoom * (w)->ximage->height / 2)
#define txth(w) (sbimg_textbox_font_height(&w->textbox) * 1.1)

#define ZOOM_AMT 1.2
#define MOVE_PCT 0.07

enum {
        TEXT = (1 << 0),
        IMAGE = (1 << 2)
};

static void sbimg_winstate_gen_ximage(
        struct sbimg_winstate *winstate,
        struct sbimg_image *image) {
        char *data;

        /* Xlib has every pixel take up 4 bytes??? idk tbh */
        data = malloc(sizeof(char) * image->width * image->height * 4);
        winstate->ximage = XCreateImage(
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
        winstate->pixmap = XCreatePixmap(
                display,
                winstate->image_window,
                image->width,
                image->height,
                DefaultDepth(display, DefaultScreen(display))
        );
        winstate->image_picture = XRenderCreatePicture(
                display,
                winstate->pixmap,
                XRenderFindVisualFormat(
                        display,
                        DefaultVisual(display, DefaultScreen(display))
                ),
                0, NULL
        );

        for (size_t x = 0; x < image->width; x++) {
                for (size_t y = 0; y < image->height; y++) {
                        struct sbimg_pixel p = sbimg_image_get_pixel(image, x, y);
                        XPutPixel(
                                winstate->ximage,
                                x, y,
                                (p.r << 16) + (p.g << 8) + (p.b << 0)
                        );
                }
        }

        XPutImage(
                display,
                winstate->pixmap,
                winstate->gc,
                winstate->ximage,
                0, 0, 0, 0,
                winstate->ximage->width,
                winstate->ximage->height
        );
}

static void sbimg_winstate_apply_transform(struct sbimg_winstate *winstate) {
        XTransform transform = {0};

        transform.matrix[0][0] = XDoubleToFixed(1/winstate->zoom);
        transform.matrix[1][1] = XDoubleToFixed(1/winstate->zoom);
        transform.matrix[2][2] = XDoubleToFixed(1.0);
        XRenderSetPictureTransform(display, winstate->image_picture, &transform);
}

void sbimg_winstate_destroy(struct sbimg_winstate *winstate) {
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
        XWindowAttributes attributes;
        struct sbimg_image image;

        winstate->files = *files;
        winstate->changes = 0;
        winstate->zoom = 1.0;
        winstate->window_width = INITIAL_WINDOW_SIZE;
        winstate->window_height = INITIAL_WINDOW_SIZE;

        winstate->window = XCreateSimpleWindow(
                display,
                DefaultRootWindow(display),
                0, 0,
                INITIAL_WINDOW_SIZE, INITIAL_WINDOW_SIZE,
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
        winstate->window_picture = XRenderCreatePicture(
                display,
                winstate->image_window,
                XRenderFindVisualFormat(
                        display,
                        DefaultVisual(display, DefaultScreen(display))
                ),
                0, NULL
        );

        sbimg_image_init(&image, sbimg_files_curr(files));
        sbimg_winstate_gen_ximage(winstate, &image);
        sbimg_image_destroy(&image);
        sbimg_winstate_apply_transform(winstate);

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

        /* 
         * Window manager may resize the window after creation,
         * and I don't seem to get a configurenotify event for this
         */
        XGetWindowAttributes(display, winstate->window, &attributes);
        winstate->window_width = attributes.width;
        winstate->window_height = attributes.height;
        winstate->center_x = attributes.width / 2;
        winstate->center_y = attributes.height / 2; }

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

void sbimg_winstate_shift_file(struct sbimg_winstate *winstate, int num) {
        struct sbimg_image image;

        winstate->changes |= IMAGE;
        winstate->changes |= TEXT;
        winstate->zoom = 1.0;
        winstate->center_x = winstate->window_width / 2;
        winstate->center_y = winstate->window_height / 2;
        sbimg_files_shift(&winstate->files, num);
        XFreePixmap(display, winstate->pixmap);
        XDestroyImage(winstate->ximage);
        sbimg_image_init(&image, sbimg_files_curr(&winstate->files));
        sbimg_winstate_gen_ximage(winstate, &image);
        sbimg_image_destroy(&image);
        sbimg_winstate_apply_transform(winstate);
}

void sbimg_winstate_translate(struct sbimg_winstate *winstate, int x, int y) {
        winstate->changes |= IMAGE;
        winstate->center_x +=
                min(winstate->window_width, winstate->ximage->width * winstate->zoom)
                * x * MOVE_PCT;
        winstate->center_y +=
                min(winstate->window_height, winstate->ximage->height * winstate->zoom)
                * y * MOVE_PCT;
}

void sbimg_winstate_zoom(struct sbimg_winstate *winstate, int p) {
        if (p < 0 && min(winstate->ximage->height, winstate->ximage->width) * winstate->zoom < 10) {
                return;
        } else if (p > 0 && max(winstate->ximage->height, winstate->ximage->width) * winstate->zoom > max(winstate->window_width, winstate->window_height) * 14) {
                return;
        }
        winstate->changes |= IMAGE;
        winstate->zoom *= pow(ZOOM_AMT, p);
        winstate->center_x = winstate->window_width / 2
                + pow(ZOOM_AMT, p) * (winstate->center_x - winstate->window_width / 2);
        winstate->center_y = winstate->window_height / 2
                + pow(ZOOM_AMT, p) * (winstate->center_y - winstate->window_height / 2);
        sbimg_winstate_apply_transform(winstate);
}

void sbimg_winstate_redraw(struct sbimg_winstate *winstate, int force_redraw) {
        char str[1024] = {0};
        int text_height = txth(winstate);

        if (winstate->changes & IMAGE || force_redraw) {
                XMoveResizeWindow(
                        display,
                        winstate->image_window,
                        tlx(winstate),
                        tly(winstate),
                        winstate->zoom * winstate->ximage->width,
                        winstate->zoom * winstate->ximage->height
                );
                XRenderComposite(
                        display,
                        PictOpOver,
                        winstate->image_picture,
                        0,
                        winstate->window_picture,
                        0, 0, 0, 0, 0, 0,
                        winstate->zoom * winstate->ximage->width,
                        winstate->zoom * winstate->ximage->height
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
