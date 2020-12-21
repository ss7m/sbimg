#include "common.h"
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
        return strcmp(sa, sb);
}

static int sbimg_has_image_ext(char *file_name) {
        char *ext = strrchr(file_name, '.');
        if (ext == NULL) {
                return false;
        }

        return strcmp(ext, ".png") == 0;
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

        files->file_count = 0;
        while ((entry = readdir(directory)) != NULL) {
                char *str;

                if (!sbimg_has_image_ext(entry->d_name)) {
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

        files->files = malloc(sizeof(char *) * files->file_count);
        rewinddir(directory);
        files->idx = -1;
        i = 0;
        while ((entry = readdir(directory)) != NULL) {
                if (!sbimg_has_image_ext(entry->d_name)) {
                        continue;
                }

                files->files[i] = malloc(sizeof(char)
                                * (2 + strlen(entry->d_name) + strlen(dir_name)));
                sprintf(files->files[i], "%s/%s", dir_name, entry->d_name);
                if (!sbimg_is_file(files->files[i])) {
                        free(files->files[i]);
                        continue;
                } else if (strcmp(file_name, entry->d_name) == 0) {
                        file_name = files->files[i];
                }
                i += 1;
        }
        qsort(files->files, files->file_count, sizeof(char *), sbimg_string_cmp);

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

void sbimg_files_prev(struct sbimg_files *files) {
        if (files->idx == 0) {
                return;
        }
        files->idx -= 1;
}

void sbimg_files_next(struct sbimg_files *files) {
        if (files->idx == files->file_count - 1) {
                return;
        }
        files->idx += 1;
}
