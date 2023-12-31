#include "../include/utilities.h"
#include <stdio.h>
#include <stdlib.h>
#include <cjson/cJSON.h>
#include <zlib.h>
#include <dirent.h>

char* utils_read_file(char* file_path) {
    FILE* f = fopen(file_path, "rb");
    if (f == NULL)
        return NULL;
    
    fseek(f, 0, SEEK_END); // go back to the end of file
    int file_size = ftell(f);
    fseek(f, 0, SEEK_SET); // return to the beginning

    char* file_contents = (char*)malloc(file_size + 1);
    size_t bytes_read = fread(file_contents, 1, file_size, f);
    fclose(f);

    if (bytes_read != file_size) {
        free(file_contents);
        return NULL;
    }

    file_contents[file_size] = 0; // null terminate
    return file_contents;
}

int utils_parse_config(char* file_path, struct config* out) {
    cfg_opt_t opts[] = {
        CFG_SIMPLE_STR("server_ip", &out->server_ip),
        CFG_SIMPLE_INT("server_port", &out->server_port),
        CFG_SIMPLE_STR("server_name", &out->server_name),
        CFG_SIMPLE_BOOL("show_server_name", &out->show_server_name),
        CFG_SIMPLE_INT("general_timeout", &out->general_timeout),
        CFG_SIMPLE_INT("max_headers_length", &out->max_headers_length),
        CFG_SIMPLE_STR("routes_json", &out->routes_json),
        CFG_SIMPLE_STR("assets_dir", &out->assets_dir),
        CFG_SIMPLE_STR("log_file", &out->log_file),
        CFG_END()
    };

    cfg_t* cfg = cfg_init(opts, 0);
    if (cfg == NULL)
        return 1;

    cfg_parse(cfg, file_path);
    cfg_free(cfg);
    return 0;
}

int utils_parse_routes(char* file_path, map* out) {
    char* data = utils_read_file(file_path);
    if (data == NULL)
        return 1;

    cJSON* root = cJSON_Parse(data);
    if (root == NULL) {
        free(data);
        return 1;
    }

    cJSON* routes_arr = cJSON_GetObjectItem(root, "routes");
    if (routes_arr == NULL) {
        cJSON_Delete(root);
        free(data);
        return 1;
    }

    for (int i = 0; i < cJSON_GetArraySize(routes_arr); i++) {
        cJSON* route_item = cJSON_GetArrayItem(routes_arr, i);
        if (route_item == NULL) {
            map_free(out);
            cJSON_Delete(root);
            free(data);
            return 1;
        }

        cJSON* path_obj = cJSON_GetObjectItem(route_item, "path");
        cJSON* render_point_obj = cJSON_GetObjectItem(route_item, "render_point");

        if (path_obj == NULL || render_point_obj == NULL) {
            map_free(out);
            cJSON_Delete(root);
            free(data);
            return 1;
        }

        char* path = cJSON_GetStringValue(path_obj);
        char* render_point = cJSON_GetStringValue(render_point_obj);

        map_push(out, path, render_point, NULL);
    }

    cJSON_Delete(root);
    free(data);
    return 0;
}

char** utils_get_dir_files(char* dir_path, int* files_len) {
    struct dirent* dir;
    DIR* d = opendir(dir_path);

    if (d == NULL)
        return NULL;
    
    size_t dir_path_len = strlen(dir_path);
    char** dir_files = NULL;
    int dir_files_len = 0;

    while ((dir = readdir(d)) != NULL) {
        if (strcmp(".", dir->d_name) != 0 && strcmp("..", dir->d_name) != 0) {
            dir_files = (char**)realloc(dir_files, (dir_files_len + 1) * sizeof(char**));
            
            dir_files[dir_files_len] = (char*)calloc(dir_path_len + dir->d_namlen + 1, 1); // + nullterminator

            strcat(dir_files[dir_files_len], dir_path);
            strcat(dir_files[dir_files_len], dir->d_name);

            dir_files_len++;
        }
    }

    closedir(d);

    *files_len = dir_files_len;
    return dir_files;
}

char* utils_gzip_compress(char* data, int* bytes_out) {
    z_stream zstrm = {0};

    if (deflateInit2(&zstrm, Z_BEST_COMPRESSION, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
        return NULL;

    int data_len = strlen(data);
    int compress_size = deflateBound(&zstrm, data_len);
    Bytef* compressed = (Bytef*)malloc(compress_size);

    zstrm.avail_in = (uInt)data_len;
    zstrm.next_in = (Bytef*)data;
    zstrm.avail_out = (uInt)compress_size;
    zstrm.next_out = (Bytef*)compressed;

    int result = deflate(&zstrm, Z_FINISH);
    deflateEnd(&zstrm);
    
    if (result == Z_OK || result == Z_STREAM_END) {
        *bytes_out = compress_size - zstrm.avail_out;
        return (char*)compressed;
    }

    *bytes_out = 0;
    free(compressed);
    return NULL;
}

map utils_load_content(char** files, int files_len) {
    map loaded_content = {0};

    for (int i = 0; i < files_len; i++) {
        char* file_bytes = utils_read_file(files[i]);
        if (file_bytes != NULL) {
            map_push(&loaded_content, files[i], file_bytes, NULL);
            free(file_bytes);
        }
    }

    return loaded_content;
}

map utils_load_content_map(map* routes) {
    map loaded_content = {0};

    for (int i = 0; i < routes->size; i++) {
        if (routes->arr[i].key != NULL && routes->arr[i].value != NULL) {
            char* file_bytes = utils_read_file(routes->arr[i].value);
            if (file_bytes != NULL) {
                map_push(&loaded_content, routes->arr[i].key, file_bytes, NULL);
                free(file_bytes);
            }
        }
    }

    return loaded_content;
}

map utils_compress_webpages(map* webpages) {
    map compressed_webpages = {0};

    for (int i = 0; i < webpages->size; i++) {
        if (webpages->arr[i].key != NULL && webpages->arr[i].value != NULL) {
            int bytes_out;
            char* compressed_bytes = utils_gzip_compress(webpages->arr[i].value, &bytes_out);
            if (compressed_bytes != NULL) {
                map_push(&compressed_webpages, webpages->arr[i].key, compressed_bytes, &bytes_out);
                free(compressed_bytes);
            }
        }
    }

    return compressed_webpages;
}

unsigned long djb2(char* str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}