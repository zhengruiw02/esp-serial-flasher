
#pragma once
#include <stdio.h>      // printf
#include <stddef.h>     // size_t

#ifdef __cplusplus
extern "C" {
#endif

// Function declarations
char* read_file_to_buffer(const char *filename, size_t *file_size);
void print_file_content(const char *buffer, size_t file_size);
void free_file_buffer(char *buffer);

#ifdef __cplusplus
}
#endif