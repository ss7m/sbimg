#ifndef SBIMG_WINDOW_H
#define SBIMG_WINDOW_H

struct sbimg_winstate {
        struct sbimg_textbox textbox;
        struct sbimg_files files;
        XImage *ximage;
        Pixmap pixmap;
        GC gc;
        Window window, image_window, text_window;
        Picture window_picture, image_picture;
        int window_width, window_height, center_x, center_y, changes;
        double zoom;
        int rotation;
};

void sbimg_winstate_destroy(struct sbimg_winstate *winstate);
void sbimg_winstate_init(
        struct sbimg_winstate *winstate,
        struct sbimg_files *files,
        const char *font_string,
        double font_size
);

void sbimg_winstate_set_dimensions(
        struct sbimg_winstate *winstate,
        int width,
        int height
);

void sbimg_winstate_shift_file(struct sbimg_winstate *winstate, int num);

void sbimg_winstate_translate(struct sbimg_winstate *winstate, int x, int y);
void sbimg_winstate_zoom(struct sbimg_winstate *winstate, int p);
void sbimg_winstate_rotate(struct sbimg_winstate *winstate, int r);

void sbimg_winstate_redraw(struct sbimg_winstate *winstate, int force_redraw);

#endif
