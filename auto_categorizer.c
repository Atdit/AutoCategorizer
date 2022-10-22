#include "auto_categorizer.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include <linux/limits.h>
#include <sys/inotify.h>
#include <sys/stat.h>

#define DIRECTORY "<directory to monitor>"
#define PICTURE_DIRECTORY "<picture directory>"
#define VIDEO_DIRECTORY "<video directory>"

int fd_inotify;
struct inotify_event *event;

void cleanup() {
    close(fd_inotify);
    free(event);
    exit(EXIT_SUCCESS);
}

#define MAX_PICTURE_FILETYPES (5)
char picture_filetypes[MAX_PICTURE_FILETYPES][16] = {
                ".jpg",
                ".jpeg",
                ".png",
                ".gif",
                ".webp"
};

#define MAX_VIDEO_FILETYPES (4)
char video_filetypes[MAX_VIDEO_FILETYPES][16] = {
                ".mp4",
                ".mkv",
                ".webm",
                ".mov"
};

void move(char *filename, char *destination, size_t filename_len, char *filetype) {
    char src_path[PATH_MAX + 1] = {0},
         des_path[PATH_MAX + 1] = {0};
    strcat(src_path, DIRECTORY);
    strcat(src_path, "/");
    strcat(src_path, filename);

    strcat(des_path, destination);
    strcat(des_path, "/");
    strcat(des_path, filename);

    printf("Moving %s\n", src_path);

    int code;

    int fail_counter = 0;
    do {
        if (fail_counter > 0) {
            snprintf(des_path, sizeof des_path - 1, "%s/%.*s-%d%s",
                     destination,
                     (int) (filename_len - strlen(filetype)),
                     filename,
                     fail_counter,
                     filetype
            );
        }
        code = access(des_path, F_OK);
        if (code == 0) {
            if (fail_counter == 0) printf("Destination name already exists. Generating new name and retrying...\n");
            fail_counter++;
        }
    } while (code == 0);

    code = rename(src_path, des_path);
    if (code != 0) {
        print_error_line("Error %d (%s) while trying to move %s to %s", errno, strerror(errno), src_path, des_path);
    } else {
        printf("Moved %s to %s\n", src_path, des_path);
    }
}

void categorize_and_move(char *filename) {
    size_t str_len = 0;
    char *filetype = NULL;
    char *current = filename;
    for (; *current != '\0'; current++) {
        str_len++;
        if (*current == '.') filetype = current;
    }
    if (filetype == NULL) return;

    for (int i = 0; i < MAX_PICTURE_FILETYPES; i++) {
        if (strcmp(filetype, picture_filetypes[i]) == 0) {
            move(filename, PICTURE_DIRECTORY, str_len, picture_filetypes[i]);
            return;
        }
    }

    for (int i = 0; i < MAX_VIDEO_FILETYPES; i++) {
        if (strcmp(filetype, video_filetypes[i]) == 0) {
            move(filename, VIDEO_DIRECTORY, str_len, video_filetypes[i]);
            return;
        }
    }
}

int main() {
    signal(SIGTERM, cleanup);
    signal(SIGINT, cleanup);

    fd_inotify = inotify_init();
    if (fd_inotify < 0) print_error_line_q("Error %d (%s) while trying to create inotify instance", errno, strerror(errno));

    int wd_close_write = inotify_add_watch(fd_inotify, DIRECTORY, IN_CLOSE_WRITE | IN_MOVED_TO);
    if (wd_close_write < 0) print_error_line_q("Error %d (%s) while trying to add inotify IN_CLOSE_WRITE watch for %s", errno, strerror(errno), DIRECTORY);

    size_t buffer_size = sizeof(struct inotify_event) + PATH_MAX + 1;
    event = malloc(buffer_size);

    while (1) {
        ssize_t bytes_read = read(fd_inotify, event, buffer_size);
        if (bytes_read < 0) print_error_line_q("Error %d (%s) while trying to read from inotify instance", errno, strerror(errno));

        char type[64] = {0};

        switch (event->mask) {
            case IN_CLOSE_WRITE:
                strcat(type, "IN_CLOSE_WRITE");
                break;
            case IN_MOVED_TO:
                strcat(type, "IN_MOVED_TO");
                break;
            default:
                strcat(type, "UNKNOWN");
                break;
        }

        printf("%s: %s\n", type, event->name);

        char path[PATH_MAX] = {0};
        strcat(path, DIRECTORY);
        strcat(path, "/");
        strcat(path, event->name);

        struct stat stats;
        int code = stat(path, &stats);
        if (code != 0) {
            continue;
        }

        categorize_and_move(event->name);
    }

    cleanup();
    return EXIT_SUCCESS;
}
