#ifndef SBIMG_FONTS_H
#define SBIMG_FONTS_H

#include <X11/Xft/Xft.h>

struct sbimg_textbox {
        XftFont *xftfont;
        XftDraw *xftdraw;
        XftColor xftcolor;
};

void sbimg_textbox_init(
        struct sbimg_textbox *textbox,
        Window window,
        const char *font_string,
        double font_size
);
void sbimg_textbox_destroy(struct sbimg_textbox *textbox);
void sbimg_textbox_write(
        struct sbimg_textbox *textbox,
        int height,
        int width,
        const char *msg
);
int sbimg_textbox_font_height(struct sbimg_textbox *textbox);

#endif
