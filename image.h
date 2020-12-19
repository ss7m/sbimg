#ifndef SBIMG_IMAGE_H
#define SBIMG_IMAGE_H

/*
 * 8-bit RGB image data
 */
struct sbimg_image {
        size_t width;
        size_t height;
        uint8_t *data;
};

struct sbimg_pixel {
        uint8_t r, g, b;
};

void sbimg_image_init(struct sbimg_image *image, const char *file_name);
void sbimg_image_destroy(struct sbimg_image *image);

struct sbimg_pixel sbimg_image_get_pixel(struct sbimg_image *image, size_t x, size_t y);
void sbimg_image_set_pixel(
        struct sbimg_image *image,
        struct sbimg_pixel pixel,
        size_t x,
        size_t y
);

#endif
