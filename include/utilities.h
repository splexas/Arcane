#ifndef UTILITIES_H
#define UTILITIES_H

#include "map.h"
#include <confuse.h>
#include <time.h>

/* All char arrays must be freed afterwards. */
struct config {
    char* server_ip;
    long int server_port;
    char* server_name;
    cfg_bool_t show_server_name;
    long int general_timeout;
    long int max_headers_length;
    char* routes_json;
    char* assets_dir;
    char* log_file;
};

/* Returns a dynamically allocated null-terminated char array. */
char* utils_read_file(char* file_path);
int utils_parse_config(char* file_path, struct config* out);
int utils_parse_routes(char* file_path, map* out);

/* returns a dynamically allocated array of dynamically allocated char arrays */
/* `dir_path` should end with `/` */
char** utils_get_dir_files(char* dir_path, int* files_len);

/* Returns a dynamically allocated char array. */
char* utils_gzip_compress(char* data, int* bytes_out);

/* mostly used for getting assets' file bytes */
map utils_load_content(char** files, int files_len);

/* its purpose is to make a map for providing static content easily (with request path in map key) */
/* `routes` must look like this - `/, index.html` */
map utils_load_content_map(map* routes);

/* basic djb2 hash to replace multiple strcmps */
unsigned long djb2(char* str);

#endif