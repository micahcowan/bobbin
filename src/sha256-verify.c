#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "sha-256.h"

void exit_with_usage(int status)
{
    FILE *fp = status == 0? stdout : stderr;

    fputs("USAGE: sha256-verify FILENAME\n"
          "\n"
          "Prints the sha-256 checksum of the specified file.\n", fp);
}

void print_sum(const uint8_t *sum, FILE *fp)
{
    for (const uint8_t *p = sum; p != sum + SIZE_OF_SHA_256_HASH; ++p) {
        fprintf(fp, "%02x", (unsigned int)*p);
    }
    fputc('\n', fp);
}

const char *progname;

int main(int argc, char **argv)
{
    if (argc != 2) exit_with_usage(2);

    progname = *argv++;
    const char *filename = *argv++;
    int fd, e;
    size_t size;

    errno = 0;
    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "%s: Could not open file \"%s\": %s\n",
                progname, filename, strerror(errno));
        exit(1);
    }

    struct stat st;
    errno = 0;
    e = fstat(fd, &st);
    if (e < 0) {
        fprintf(stderr, "%s: Could not stat file \"%s\": %s\n",
                progname, filename, strerror(errno));
        exit(1);
    }
    size = st.st_size;

    errno = 0;
    unsigned char * contents = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (contents == NULL) {
        fprintf(stderr, "%s: mmap failed: %s\n",
                progname, strerror(errno));
        exit(1);
    }

    uint8_t hash[SIZE_OF_SHA_256_HASH];
    calc_sha_256(hash, contents, size);

    print_sum(hash, stdout);

    return 0;
}
