#include <stdio.h>      // printf
#include <stdint.h>     // uint8_t
#include <stdlib.h>     // free
#include <stddef.h>     // size_t
#include <errno.h>      // error code
#include <sys/stat.h>   // fstat
#include <string.h>     // strerror
#include <time.h>       // gettime

#include "esp_loader.h"
#include "example_common.h"

/**
 * Read file content into dynamic memory
 * @param filename Name of the file to read
 * @param file_size Pointer to store the file size
 * @return Pointer to buffer on success, NULL on failure
 */
char* read_file_to_buffer(const char *filename, size_t *file_size) {
    // Parameter validation
    if (!filename || !file_size) {
        fprintf(stderr, "Invalid parameters\n");
        return NULL;
    }

    // Open file
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Error opening file '%s': %s\n", filename, strerror(errno));
        return NULL;
    }

    // Get file size
    struct stat st;
    if (fstat(fileno(file), &st) != 0) {
        fprintf(stderr, "Error getting file size: %s\n", strerror(errno));
        fclose(file);
        return NULL;
    }

    *file_size = st.st_size;
    printf("File size: %zu bytes\n", *file_size);

    // Handle empty file
    if (*file_size == 0) {
        fclose(file);
        return NULL;
    }

    // Allocate memory
    char *buffer = malloc(*file_size);
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        return NULL;
    }

    // Read file content
    size_t bytes_read = fread(buffer, 1, *file_size, file);
    if (bytes_read != *file_size) {
        fprintf(stderr, "Error: expected %zu bytes, read %zu bytes\n", *file_size, bytes_read);
        if (ferror(file)) {
            fprintf(stderr, "File read error: %s\n", strerror(errno));
        }
        free(buffer);
        fclose(file);
        return NULL;
    }

    fclose(file);
    return buffer;
}

/**
 * Print file content (first 100 characters)
 * @param buffer File content buffer
 * @param file_size Size of the file
 */
void print_file_content(const char *buffer, size_t file_size) {
    if (!buffer) {
        printf("No content to print\n");
        return;
    }

    if (file_size == 0) {
        printf("File is empty\n");
        return;
    }

    printf("First 100 characters:\n");
    size_t print_size = (file_size < 100) ? file_size : 100;
    
    // Use fwrite to avoid string processing issues
    fwrite(buffer, 1, print_size, stdout);
    printf("\n");
}

/**
 * Free file buffer memory
 * @param buffer Buffer to be freed
 */
void free_file_buffer(char *buffer) {
    if (buffer) {
        free(buffer);
        printf("Memory freed successfully\n");
    }
}

int read_bin_and_flash(const char *filename, size_t flash_address)
{
    char *buffer = NULL;
    size_t file_size = 0;
    struct timespec start_time, end_time;
    // Record start time
    if (clock_gettime(CLOCK_MONOTONIC, &start_time) == -1) {
        perror("clock_gettime start");
        return -1;
    }
    printf("Flash adress: 0x%zx \n", flash_address);
    
    buffer = read_file_to_buffer(filename, &file_size);
    if(buffer == NULL){
        return -1;
    }

    // successfully read buffer from file

    // print_file_content(buffer, file_size);
    flash_binary(buffer, file_size, flash_address);

    free_file_buffer(buffer);

    // Record end time
    if (clock_gettime(CLOCK_MONOTONIC, &end_time) == -1) {
        perror("clock_gettime end");
        return -1;
    }

    // Calculate elapsed time
    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000000000.0 +
                         (end_time.tv_nsec - start_time.tv_nsec);
    // Convert to different units for better readability
    printf("Print time: %.3f seconds\n", elapsed_time / 1000000000.0);

    return 0;
}