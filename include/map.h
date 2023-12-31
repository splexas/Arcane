#ifndef MAP_H
#define MAP_H

typedef struct _map_node {
    char* key;
    char* value;
    int value_len;
} map_node;

typedef struct _map {
    map_node* arr;
    int size;
} map;

/* `value_len` is optional. */
/* `map_free` must be called after the map is no longer in use. */
void map_push(map* map, char* key, char* value, int* value_len);

void map_remove(map* map, char* key);
map_node* map_find(map* map, char* key);
char* map_find_value(map* map, char* key);

/* Duplicates `src_map` with fully independent data. */
/* If `src_map` is `NULL` or its contents are `NULL` or `0`, its returned map contents are guaranteed to be `NULL` or `0`. */
map map_dup(map* src_map);

int map_get_pairs_size(map* map);
void map_free(map* map);

#endif