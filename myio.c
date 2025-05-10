#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#define BLOCK_SIZE 4096

// Comparison function for qsort
int compare(const void *a, const void *b) {
    double diff = (*(double *)a - *(double *)b);
    return (diff > 0) - (diff < 0);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <mode> <filename> <file_size/io_size> [samples]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int mode = atoi(argv[1]);
    char *filename = argv[2];

    if (mode == 1) { // File Creation Mode
        if (argc != 4) {
            fprintf(stderr, "Usage: %s 1 <filename> <file_size>\n", argv[0]);
            exit(EXIT_FAILURE);
        }
        long file_size = atol(argv[3]);
        if (file_size <= 0) {
            fprintf(stderr, "Invalid file size\n");
            exit(EXIT_FAILURE);
        }

        // Open file with truncation, read/write permissions for owner/group, read for others
        int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
        if (fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        // Allocate buffer and fill with zeros
        char *buffer = (char *)malloc(BLOCK_SIZE);
        if (!buffer) {
            perror("malloc");
            close(fd);
            exit(EXIT_FAILURE);
        }
        memset(buffer, 0, BLOCK_SIZE);

        // Write file_size blocks
        for (long i = 0; i < file_size; i++) {
            if (write(fd, buffer, BLOCK_SIZE) != BLOCK_SIZE) {
                perror("write");
                free(buffer);
                close(fd);
                exit(EXIT_FAILURE);
            }
        }

        // Sync to disk
        if (fsync(fd) == -1) {
            perror("fsync");
            free(buffer);
            close(fd);
            exit(EXIT_FAILURE);
        }

        free(buffer);
        close(fd);

    } else if (mode == 2) { // Random IO Mode
        if (argc != 5) {
            fprintf(stderr, "Usage: %s 2 <filename> <io_size> <samples>\n", argv[0]);
            exit(EXIT_FAILURE);
        }
        long io_size = atol(argv[3]);
        long samples = atol(argv[4]);
        if (io_size <= 0 || samples <= 0) {
            fprintf(stderr, "Invalid io_size or samples\n");
            exit(EXIT_FAILURE);
        }

        // Get file size
        struct stat st;
        if (stat(filename, &st) == -1) {
            perror("stat");
            exit(EXIT_FAILURE);
        }
        off_t file_size = st.st_size;
        if (file_size < io_size) {
            fprintf(stderr, "File size too small for io_size\n");
            exit(EXIT_FAILURE);
        }

        // Open file for reading
        int fd = open(filename, O_RDONLY);
        if (fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        // Allocate buffer for reading
        char *buffer = (char *)malloc(io_size);
        if (!buffer) {
            perror("malloc");
            close(fd);
            exit(EXIT_FAILURE);
        }

        // Allocate array for latencies
        double *latencies = (double *)malloc(samples * sizeof(double));
        if (!latencies) {
            perror("malloc");
            free(buffer);
            close(fd);
            exit(EXIT_FAILURE);
        }

        // Initialize random number generator
        srandom(time(NULL));

        // Perform random IO
        struct timespec start, end;
        long max_offset = file_size - io_size;
        long num_blocks = max_offset / BLOCK_SIZE + (max_offset % BLOCK_SIZE ? 1 : 0);
        for (long i = 0; i < samples; i++) {
            // Generate random block-aligned offset
            long block = random() % num_blocks;
            off_t offset = block * BLOCK_SIZE;

            // Measure time for lseek + read
            clock_gettime(CLOCK_MONOTONIC, &start);
            if (lseek(fd, offset, SEEK_SET) == -1) {
                perror("lseek");
                free(buffer);
                free(latencies);
                close(fd);
                exit(EXIT_FAILURE);
            }
            if (read(fd, buffer, io_size) != io_size) {
                perror("read");
                free(buffer);
                free(latencies);
                close(fd);
                exit(EXIT_FAILURE);
            }
            clock_gettime(CLOCK_MONOTONIC, &end);

            // Calculate latency in microseconds
            double latency = (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_nsec - start.tv_nsec) / 1e3;
            latencies[i] = latency;
        }

        // Sort latencies to find median
        qsort(latencies, samples, sizeof(double), compare);
        double median;
        if (samples % 2 == 0) {
            median = (latencies[samples / 2 - 1] + latencies[samples / 2]) / 2.0;
        } else {
            median = latencies[samples / 2];
        }

        // Print median latency with two decimal places
        printf("%.2f\n", median);

        free(buffer);
        free(latencies);
        close(fd);

    } else {
        fprintf(stderr, "Invalid mode\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}
