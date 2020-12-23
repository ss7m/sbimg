#include "common.h"
#include "image.h"
#include "files.h"

#include <dirent.h>
#include <sys/stat.h>

static int sbimg_is_file(char *file_name) {
        struct stat file_stat;
        stat(file_name, &file_stat);
        return S_ISREG(file_stat.st_mode);
}

static int sbimg_string_cmp(const void *a, const void *b) {
        char *sa = *(char **)a;
        char *sb = *(char **)b;
        char ca, cb;

        do {
                ca = *sa++;
                cb = *sb++;
                if (ca == '\0' || ca == '.') {
                        return ca - cb;
                } 
        } while (ca == cb);
        return ca - cb;
}

void sbimg_files_destroy(struct sbimg_files *files) {
        int i;
        for (i = 0; i < files->file_count; i++) {
                free(files->files[i]);
        }
        free(files->files);
}

void sbimg_files_init(struct sbimg_files *files, char *file_name) {
        DIR *directory;
        struct dirent *entry;
        char *file_name_copy, *dir_name;
        int i;

        file_name_copy = malloc(sizeof(char) * (1 + strlen(file_name)));
        strcpy(file_name_copy, file_name);
        dir_name = dirname(file_name_copy);
        directory = opendir(dir_name);
        file_name = basename(file_name);

        /* count image files in the directory */
        files->file_count = 0;
        while ((entry = readdir(directory)) != NULL) {
                char *str;

                if (sbimg_parse_file_ext(entry->d_name) == IMAGE_TYPE_NONE) {
                        continue;
                }
                str = malloc(
                        sizeof(char) * (2 + strlen(entry->d_name) + strlen(dir_name))
                );
                sprintf(str, "%s/%s", dir_name, entry->d_name);
                if (!sbimg_is_file(str)) {
                        free(str);
                        continue;
                }
                free(str);
                files->file_count += 1;
        }

        /* collect the names of the image files */
        files->files = malloc(sizeof(char *) * files->file_count);
        rewinddir(directory);
        files->idx = -1;
        i = 0;
        while ((entry = readdir(directory)) != NULL) {
                if (sbimg_parse_file_ext(entry->d_name) == IMAGE_TYPE_NONE) {
                        continue;
                }

                files->files[i] = malloc(sizeof(char)
                                * (2 + strlen(entry->d_name) + strlen(dir_name)));
                sprintf(files->files[i], "%s/%s", dir_name, entry->d_name);
                if (!sbimg_is_file(files->files[i])) {
                        free(files->files[i]);
                        continue;
                } else if (strcmp(file_name, entry->d_name) == 0) {
                        /* get full path of file_name so that we can look it up later */
                        file_name = files->files[i];
                }
                i += 1;
        }

        /* sort alphabetically */
        qsort(files->files, files->file_count, sizeof(char *), sbimg_string_cmp);

        /* look up idx of file_name */
        i = 0;
        do {
                if (i >= files->file_count) {
                        sbimg_error("%s not a file or has wrong extension\n", file_name);
                }
        } while (strcmp(file_name, files->files[i++]) != 0);
        files->idx = i - 1;

        free(file_name_copy);
        free(directory);
}

void sbimg_files_shift(struct sbimg_files *files, int num) {
        files->idx = min(
                files->file_count - 1,
                max(0, files->idx + num)
        );
}
