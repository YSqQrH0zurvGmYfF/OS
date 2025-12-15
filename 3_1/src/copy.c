#include "copy.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define ERROR -1

#ifndef BUF_SIZE
#define BUF_SIZE 8192
#endif


static int copy_file_data(int in_fd, int out_fd) {
    uint8_t buf[BUF_SIZE];

    while (1) {
        ssize_t nr = read(in_fd, buf, sizeof(buf));
        
        if (nr == ERROR && errno == EINTR) { continue; }
        if (nr == ERROR) { return COPY_IO_FAILURE; }
        if (nr == 0) { break; }

        ssize_t total_written = 0;
        while (total_written < nr) {
            ssize_t nw = write(out_fd, buf + total_written,
                               (size_t)(nr - total_written));

            if (nw == ERROR && errno == EINTR) { continue; }
            if (nw == ERROR) { return COPY_IO_FAILURE; }

            total_written += nw;
        }
    }

    return COPY_SUCCESS;
}

int copy_file(const char *src, const char *dst, int mode) {
    int status = COPY_SUCCESS;
    int in_fd = -1, out_fd = -1;

    in_fd = open(src, O_RDONLY);
    if (in_fd == ERROR) { return COPY_OPEN_FAILURE; }

    unlink(dst);

    out_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (out_fd == ERROR) {
        close(in_fd);
        return COPY_OPEN_FAILURE;
    }

    status = copy_file_data(in_fd, out_fd);
    if (status != COPY_SUCCESS) { goto exit; }

    int err = fchmod(out_fd, mode);
    if (err == ERROR) {
        status = COPY_MODE_CHANGE_FAILURE;
        goto exit;
    }

exit:
    close(in_fd);
    close(out_fd);
    return status;
}

int mkdir_with_mode(const char* dir, int mode) {
    int status;

    if (!S_ISDIR(mode)) { return COPY_INVALID_ARGUMENT; }

    status = mkdir(dir, S_IRWXU);
    if (status == ERROR && errno != EEXIST) {
        return COPY_FAILURE;
    }
    
    status = chmod(dir, mode); 
    if (status == ERROR) {
        return COPY_MODE_CHANGE_FAILURE;
    }

    return COPY_SUCCESS;
}

