#ifndef SBIMG_IMAGE_H
#define SBIMG_IMAGE_H

enum sbimg_image_type {
        IMAGE_TYPE_NONE,
        IMAGE_TYPE_JPG,
        IMAGE_TYPE_PNG
};

/*
 * 8-bit RGB image data
 */
struct sbimg_image {
        enum {
                SBIMG_RGB,
                SBIMG_GRAY
        } pixel_type;
        size_t width;
        size_t height;
        uint8_t *data;
};

struct sbimg_pixel {
        uint8_t r, g, b;
};

enum sbimg_image_type sbimg_parse_file_ext(const char *file_name);
void sbimg_image_init(struct sbimg_image *image, const char *file_name);
void sbimg_image_destroy(struct sbimg_image *image);

struct sbimg_pixel sbimg_image_get_pixel(struct sbimg_image *image, size_t x, size_t y);

#endif
