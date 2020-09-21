#include <stdio.h>
#include <string.h>
#include "cwalk/cwalk.h"
#include "path.h"

int path_file_exists(char *filename) {
    FILE *file;
    if ((file = fopen(filename, "r")) != NULL) {
        fclose(file);
        return 1;
    }
    return 0;
}

void path_get_output_filename(char *buffer, size_t buffer_length, const char *output_dir, const char *filename) {
    const char *basename;
    size_t basename_length;
    cwk_path_get_basename(filename, &basename, &basename_length);

    char temp_buffer[basename_length + 1];
    memcpy(&temp_buffer, basename, basename_length);
    temp_buffer[basename_length] = '\0';

    cwk_path_join(output_dir, temp_buffer, buffer, buffer_length);
}
