#ifndef SBIMG_FILES_H
#define SBIMG_FILES_H

struct sbimg_files {
        int file_count, idx;
        char **files;
};

void sbimg_files_destroy(struct sbimg_files *files);
void sbimg_files_init(struct sbimg_files *files, char *file_name);
void sbimg_files_prev(struct sbimg_files *files);
void sbimg_files_next(struct sbimg_files *files);
void sbimg_files_curr(struct sbimg_files *files);

#define sbimg_files_curr(f) ((f)->files[(f)->idx])

#endif
