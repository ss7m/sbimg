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
                if (ca == '\0') {
                        return ca - cb;
                } else if (ca == '.' && cb != '.') {
                        return -1;
                } else if (ca != '.' && cb == '.') {
                        return 1;
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
        char *dir_name, *temp1, *temp2;
        int files_capacity;

        /* get directory name and open directory */
        temp1 = malloc(sizeof(char) * (1 + strlen(file_name)));
        strcpy(temp1, file_name);
        dir_name = dirname(temp1);
        directory = opendir(dir_name);

        /* format file_name with dir_name */
        file_name = basename(file_name);
        temp2 = malloc(2 + strlen(file_name) + strlen(dir_name));
        sprintf(temp2, "%s/%s", dir_name, file_name);
        file_name = temp2;

        files_capacity = 4;
        files->file_count = 0;
        files->files = malloc(sizeof(char *) * files_capacity);

        /* count image files in the directory */
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
                if (files->file_count >= files_capacity) {
                        files_capacity *= 1.5;
                        files->files = realloc(
                                files->files,
                                files_capacity * sizeof(char *)
                        );
                }
                files->files[files->file_count] = str;
                files->file_count += 1;
        }

        /* sort alphabetically */
        qsort(files->files, files->file_count, sizeof(char *), sbimg_string_cmp);

        /* look up idx of file_name */
        files->idx = 0;
        do {
                if (files->idx >= files->file_count) {
                        sbimg_error("%s not a file or has wrong extension\n", file_name);
                }
        } while (strcmp(file_name, files->files[files->idx++]) != 0);
        files->idx--;

        free(temp1);
        free(temp2);
        free(directory);
}

void sbimg_files_shift(struct sbimg_files *files, int num) {
        files->idx = min(
                files->file_count - 1,
                max(0, files->idx + num)
        );
}
