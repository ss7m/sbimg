#include "common.h"
#include "fonts.h"

#define get_width(textbox) ((textbox)->xftfont->max_advance_width)
#define get_height(textbox) ((textbox)->xftfont->height)
#define get_ascent(textbox) ((textbox)->xftfont->ascent)
#define get_descent(textbox) ((textbox)->xftfont->descent)

void sbimg_textbox_init(
        struct sbimg_textbox *textbox,
        Window window,
        const char *font_string,
        double font_size)
{
        XRenderColor xrcolor;
        textbox->xftdraw = XftDrawCreate(
                display,
                window,
                DefaultVisual(display, DefaultScreen(display)),
                DefaultColormap(display, DefaultScreen(display))
        );
        textbox->xftfont = XftFontOpen(
                display,
                DefaultScreen(display),
                XFT_FAMILY, XftTypeString, font_string,
                XFT_SIZE, XftTypeDouble, font_size,
                NULL
        );

        xrcolor.red = xrcolor.green = xrcolor.blue = xrcolor.alpha = 0xFFFF;
        XftColorAllocValue(
                display,
                DefaultVisual(display, DefaultScreen(display)),
                DefaultColormap(display, DefaultScreen(display)),
                &xrcolor,
                &textbox->xftcolor
        );
}

void sbimg_textbox_destroy(struct sbimg_textbox *textbox) {
        XftColorFree(
                display,
                DefaultVisual(display, DefaultScreen(display)),
                DefaultColormap(display, DefaultScreen(display)),
                &textbox->xftcolor
        );
        XftFontClose(display, textbox->xftfont);
        XftDrawDestroy(textbox->xftdraw);
}

int sbimg_textbox_font_height(struct sbimg_textbox *textbox) {
        return get_height(textbox);
}

/*
 * height: height of the space we're drawing insie
 * width: if 0, left align, if > 0, right align at width
 */
void sbimg_textbox_write(
        struct sbimg_textbox *textbox,
        int height,
        int width,
        const char *msg)
{
        int x = (width == 0) ? (get_width(textbox))
                : (width - ((int) strlen(msg) + 1) * get_width(textbox));
        int y = get_ascent(textbox) + (height - get_height(textbox)) / 2;
        XftDrawString8(
                textbox->xftdraw,
                &textbox->xftcolor,
                textbox->xftfont,
                x, y,
                (FcChar8 *)msg,
                strlen(msg)
        );
}
