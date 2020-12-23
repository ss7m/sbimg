#include "common.h"
#include "image.h"

#include <jpeglib.h>
#include <png.h>

#define SBIMG_PNG_VERSION "1.6.37"
#define PNG_SIG_LENGTH 8

#define sbimg_image_num_components(image) ((image)->pixel_type == SBIMG_GRAY ? 1 : 3)
#define sbimg_image_row_size(image) ((image)->width \
                * sbimg_image_num_components(image) * sizeof(uint8_t))
#define sbimg_image_xy(image, x, y) ((y) * sbimg_image_row_size(image) \
                + (x) * sbimg_image_num_components(image) * sizeof(uint8_t))

enum sbimg_image_type sbimg_parse_file_ext(const char *file_name) {
        char *ext = strrchr(file_name, '.');

        if (ext == NULL) {
                return IMAGE_TYPE_NONE;
        } else if (strcmp(ext, ".png") == 0) {
                return IMAGE_TYPE_PNG;
        } else if (strcmp(ext, ".jpg") == 0) {
                return IMAGE_TYPE_JPG;
        } else if (strcmp(ext, ".jpeg") == 0) {
                return IMAGE_TYPE_JPG;
        } else {
                return IMAGE_TYPE_NONE;
        }
}

/* Reading a JPG file */

static void sbimg_read_jpg_file(struct sbimg_image *image, const char *file_name) {
        FILE *file;
        struct jpeg_decompress_struct cinfo;
        struct jpeg_error_mgr jerror;
        JSAMPARRAY buffer;
        unsigned int i, row_size; /* row width in buffer */

        if ((file = fopen(file_name, "rb")) == NULL) {
                sbimg_error("Unable to open file %s for reading\n", file_name);
        }

        cinfo.err = jpeg_std_error(&jerror);
        jpeg_create_decompress(&cinfo);
        jpeg_stdio_src(&cinfo, file);
        jpeg_read_header(&cinfo, true);
        jpeg_start_decompress(&cinfo);
        
        row_size = cinfo.output_width * cinfo.output_components;
        image->data = malloc(row_size * cinfo.output_height);
        image->width = cinfo.output_width;
        image->height = cinfo.output_height;
        image->pixel_type = cinfo.output_components == 1 ? SBIMG_GRAY : SBIMG_RGB;
        buffer = cinfo.mem->alloc_sarray(
                (j_common_ptr) &cinfo,
                JPOOL_IMAGE,
                row_size,
                1
        );

        for(i = 0; i < cinfo.output_height; i++) {
                jpeg_read_scanlines(&cinfo, buffer, 1);
                memcpy(image->data + i * row_size, *buffer, row_size);
        }

        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);
        fclose(file);
}

/* Reading a PNG file */

struct sbimg_png_reader {
        png_struct *png;
        png_info *info; };

/*
 * Reads the first 8 bytes and checks them against the magic png header
 * Should probably be followed with a call to "png_set_sig_bytes"
 */
static int sbimg_check_if_png(FILE *file) {
        uint8_t bytes[PNG_SIG_LENGTH];
        fread(bytes, sizeof(uint8_t), PNG_SIG_LENGTH, file);
        return png_sig_cmp(bytes, 0, PNG_SIG_LENGTH) == 0;
}

/*
 * struct sbimg_png reading functions
 */

static void sbimg_png_reader_init(
        struct sbimg_png_reader *reader,
        const char *file_name)
{
        png_struct *png;
        png_info *info;
        FILE *file;
        png_color_16 background;

        png = png_create_read_struct(SBIMG_PNG_VERSION, NULL, NULL, NULL);
        if (png == NULL) {
                sbimg_error("Unable to create read struct\n");
        }

        info = png_create_info_struct(png);
        if (info == NULL) {
                sbimg_error("Unable to create info struct\n");
        }

        file = fopen(file_name, "rb");
        if (file == NULL) {
                sbimg_error("Unable to open file %s for reading\n", file_name);
        } else if (!sbimg_check_if_png(file)) {
                sbimg_error("File %s is not a png file\n", file_name);
        }

        background.red = background.green = background.blue = 0xFF;
        png_set_background(png, &background, PNG_BACKGROUND_GAMMA_SCREEN, 0, 2.2);
        png_init_io(png, file);
        png_set_sig_bytes(png,PNG_SIG_LENGTH);
        png_read_png(
                png, info,
                PNG_TRANSFORM_STRIP_16 |
                PNG_TRANSFORM_PACKING |
                PNG_TRANSFORM_GRAY_TO_RGB |
                PNG_TRANSFORM_STRIP_ALPHA,
                NULL
        );

        reader->png = png;
        reader->info = info;
        fclose(file);
}

static void sbimg_png_reader_destroy(struct sbimg_png_reader *reader) {
        png_destroy_read_struct(&reader->png, &reader->info, NULL);
}

/*
 * Load an image from a png reader
 */
static void sbimg_png_reader_load_image(
        struct sbimg_png_reader *reader,
        struct sbimg_image *image)
{
        size_t i, row_size;
        uint8_t **image_rows;

        image->width = png_get_image_width(reader->png, reader->info);
        image->height = png_get_image_height(reader->png, reader->info);
        image->pixel_type = SBIMG_RGB;
        row_size = sbimg_image_row_size(image);
        image->data = malloc(row_size * image->height);

        image_rows = png_get_rows(reader->png, reader->info);
        for (i = 0; i < image->height; i++) {
                memcpy(image->data + i * row_size, image_rows[i], row_size);
        }
}

/*
 * struct sbimg_image functions
 */

void sbimg_image_init(struct sbimg_image *image, const char *file_name) {
        struct sbimg_png_reader reader;
        switch(sbimg_parse_file_ext(file_name)) {
        case IMAGE_TYPE_PNG:
                sbimg_png_reader_init(&reader, file_name);
                sbimg_png_reader_load_image(&reader, image);
                sbimg_png_reader_destroy(&reader);
                break;
        case IMAGE_TYPE_JPG:
                sbimg_read_jpg_file(image, file_name);
                break;
        case IMAGE_TYPE_NONE:
                sbimg_error("%s has a bad extension\n", file_name);
        }
}

void sbimg_image_destroy(struct sbimg_image *image) {
        free(image->data);
}

struct sbimg_pixel sbimg_image_get_pixel(struct sbimg_image *image, size_t x, size_t y) {
        struct sbimg_pixel pixel;
        switch (image->pixel_type) {
        case SBIMG_RGB:
                pixel.r = image->data[sbimg_image_xy(image, x, y)];
                pixel.g = image->data[sbimg_image_xy(image, x, y) + 1];
                pixel.b = image->data[sbimg_image_xy(image, x, y) + 2];
                break;
        case SBIMG_GRAY:
                pixel.r = pixel.g = pixel.b = image->data[sbimg_image_xy(image, x, y)];
                break;
        }
        return pixel;
}
