#include "../include/updater.h"
#include "../include/utilities.h"
#include "../include/logger.h"
#include "../include/arcane.h" // for config

#ifdef _WIN32
#include <fileapi.h>
#include <handleapi.h>
#include <synchapi.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <stdlib.h>
#include <stdio.h>

time_t updater_get_last_modified_time(char* file_path) {
    #ifdef _WIN32
    HANDLE f = CreateFileA(file_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == NULL)
        return 0;

    FILETIME last_write_time;    
    WINBOOL status = GetFileTime(f, NULL, NULL, &last_write_time);
    CloseHandle(f);

    if (status == FALSE)
        return 0;

    ULARGE_INTEGER uli;
    uli.LowPart = last_write_time.dwLowDateTime;
    uli.HighPart = last_write_time.dwHighDateTime;

    return uli.QuadPart / 10000000ULL - 11644473600ULL;
    #else
    struct stat attr;
    if (stat(file_path, &attr) != 0)
        return 0;

    return attr.st_mtime;
    #endif
}

void updater_update(void* args) {
    struct updater_args* upd_args = (struct updater_args*)args;

    time_t* webpage_last_modified_times = NULL;
    time_t* assets_last_modified_times = NULL;

    while (1) {

        /* load webpages last modified times */

        if (webpage_last_modified_times == NULL) {
            webpage_last_modified_times = (time_t*)malloc(upd_args->static_content->size * sizeof(time_t));
            for (int i = 0; i < upd_args->static_content->size; i++)
                webpage_last_modified_times[i] = updater_get_last_modified_time(upd_args->static_content->arr[i].key);

                                
            #ifdef DEBUG
            ARCANE_LOG(config.log_file, ARC_DEBUG, arc_false, "[updater_update] Renewed webpage files last modified times\n");
            #endif
        }

        /* check if any new entries were added in json */

        map new_route_map = {0};

        if (utils_parse_routes(upd_args->static_content_routes_file, &new_route_map) == 0) {
            if (new_route_map.size != upd_args->static_content->size) {

                map new_static_content = utils_load_content_map(&new_route_map);
                if (new_static_content.arr != NULL) {

                    map_free(upd_args->static_content);
                    *upd_args->static_content = new_static_content;
                    
                    #ifdef DEBUG
                    ARCANE_LOG(config.log_file, ARC_DEBUG, arc_false, "[updater_update] Replaced static webpages' contents\n");
                    #endif

                }

                free(webpage_last_modified_times);
                webpage_last_modified_times = NULL;
                continue;
            
            }

            map_free(&new_route_map);
        }

        /* check if static contents were updated */

        for (int i = 0; i < upd_args->static_content->size; i++) {
            time_t modified_time = updater_get_last_modified_time(upd_args->static_content->arr[i].key);
            if (modified_time > webpage_last_modified_times[i]) {    

                #ifdef DEBUG
                ARCANE_LOG(config.log_file, ARC_DEBUG, arc_false, "[updater_update] Detected static webpage file change\n");
                #endif

                char* contents = utils_read_file(upd_args->static_content->arr[i].key);

                if (contents != NULL) {
                    free(upd_args->static_content->arr[i].value);
                    upd_args->static_content->arr[i].value = contents;
                    upd_args->static_content->arr[i].value_len = strlen(contents);
                
                    #ifdef DEBUG
                    ARCANE_LOG(config.log_file, ARC_DEBUG, arc_false, "[updater_update] Replaced static webpage content\n");
                    #endif
                }

                webpage_last_modified_times[i] = modified_time;
            }
        }

        /* ------------------- */

        if (upd_args->static_assets->arr != NULL) {
            
            /* load assets last modified times */

            if (assets_last_modified_times == NULL) {
                assets_last_modified_times = (time_t*)malloc(upd_args->static_assets->size * sizeof(time_t));

                for (int i = 0; i < upd_args->static_assets->size; i++)
                    assets_last_modified_times[i] = updater_get_last_modified_time(upd_args->static_assets->arr[i].key);

                #ifdef DEBUG
                ARCANE_LOG(config.log_file, ARC_DEBUG, arc_false, "[updater_update] Renewed assets last modified times\n");
                #endif
            }

            int new_asset_files_len = 0;
            char** new_asset_files = utils_get_dir_files(upd_args->static_assets_dir, &new_asset_files_len);

            if (new_asset_files != NULL) {      

                /* check if new assets were added */

                if (new_asset_files_len != upd_args->static_assets->size) {

                    map new_assets_content = utils_load_content(new_asset_files, new_asset_files_len);
                    if (new_assets_content.arr != NULL) {

                        map_free(upd_args->static_assets);
                        *upd_args->static_assets = new_assets_content;
                        
                        #ifdef DEBUG
                        ARCANE_LOG(config.log_file, ARC_DEBUG, arc_false, "[updater_update] Replaced assets content\n");
                        #endif

                        free(assets_last_modified_times);
                        assets_last_modified_times = NULL;
                        continue;
                    }
                }

                /* check if assets were modified */

                for (int i = 0; i < new_asset_files_len; i++) {
                    time_t modified_time = updater_get_last_modified_time(new_asset_files[i]);
                    if (modified_time > assets_last_modified_times[i]) {

                        #ifdef DEBUG
                        ARCANE_LOG(config.log_file, ARC_DEBUG, arc_false, "[updater_update] Detected static asset file change\n");
                        #endif
                        
                        char* contents = utils_read_file(new_asset_files[i]);
                        if (contents != NULL) {
                            free(upd_args->static_assets->arr[i].value);
                            upd_args->static_assets->arr[i].value = contents;
                            upd_args->static_assets->arr[i].value_len = strlen(contents);
                        }

                        assets_last_modified_times[i] = modified_time;
                    }

                    free(new_asset_files[i]);
                }

                free(new_asset_files);
            }

        }


        #ifdef _WIN32
        Sleep(1000);
        #else
        sleep(1);
        #endif
    }

    free(upd_args);
    
    if (webpage_last_modified_times != NULL)
        free(webpage_last_modified_times);

    if (assets_last_modified_times != NULL)
        free(assets_last_modified_times);
}