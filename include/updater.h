#ifndef UPDATER_H
#define UPDATER_H

#include "map.h"
#include <time.h>

struct updater_args {
    char* static_content_routes_file;
    map* static_content;
    char* static_assets_dir;
    map* static_assets;
};

time_t updater_get_last_modified_time(char* file_path);

/* `args` must be dynamically allocated. The function inherits `args` and frees it later. */
void updater_update(void* args); // thread

#endif