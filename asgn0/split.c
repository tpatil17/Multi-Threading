#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <err.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

    // not enough arguments
    if (argc < 3) {

        errx(22, "Not enough arguments\nusage: ./split: <split_char> [<file1> "
                 "<file2> ...]");
    }

    if (strlen(argv[1]) > 1) {

        errx(22,
            "Cannot handle multi-character splits: %s\nusage: ./split: "
            "<split_char> [<file1> "
            "<file2> ...]",
            argv[1]);
    }

    int i = 1;

    int red;

    char *delim = argv[1];

    int bytes_exp;

    char *buffer;

    struct stat buf;

    i++;
    while (i < argc) {

        int fd = 2;

        if (strcmp(argv[i], "-") == 0) {
            fd = 0;
        }

        else {
            fd = open(argv[i], O_RDONLY);
        }

        if (fd == -1) {

            errx(2, "split: %s: No such file or directory", argv[i]);
        }

        // When input is stdin

        if (fd == 0) {

            char buffer_2[1024];

            while ((red = read(fd, buffer_2, 1023)) > 0) {

                for (int ctr = 0; ctr < red; ctr++) {
                    if (buffer_2[ctr] == delim[0]) {
                        buffer_2[ctr] = '\n';
                    }
                }

                buffer_2[red] = '\0';

                if ((write(STDOUT_FILENO, buffer_2, red)) == -1) {
                    perror("write");
                    exit(1);
                }

                if (red == -1) {
                    perror("read");
                    exit(1);
                }
            }

            if (close(fd) == -1) {
                perror("close");
                exit(1);
            }
        }

        // other cases

        else {

            fstat(fd, &buf);

            bytes_exp = buf.st_size;

            buffer = (char *) calloc(bytes_exp + 1, sizeof(char));

            if (buffer == NULL) {
                errx(2, "Memory not allocated.\n");
            }

            red = read(fd, buffer, bytes_exp);

            for (int ctr = 0; ctr < red; ctr++) {
                if (buffer[ctr] == delim[0]) {
                    buffer[ctr] = '\n';
                }
            }

            buffer[red] = '\0';

            if ((write(STDOUT_FILENO, buffer, red)) == -1) {
                perror("write");
                exit(1);
            }

            if (red == -1) {
                perror("read");
                exit(1);
            }

            if (close(fd) == -1) {
                perror("close");
                exit(1);
            }

            free(buffer);
        }

        i++;
    }

    return 0;
}
