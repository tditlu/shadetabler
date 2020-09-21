#ifndef _PATH_H
#define _PATH_H

int path_file_exists(char *filename);
void path_get_output_filename(char *buffer, size_t buffer_length, const char *output_dir, const char *filename);

#endif /* _PATH_H */
