#include "../include/map.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void map_push(map* map, char* key, char* value, int* value_len) {
    map->arr = (map_node*)realloc(map->arr, sizeof(map_node) * (map->size + 1));
    if (map->arr == NULL)
        return;

    map->arr[map->size].key = strdup(key);

    int len;
    if (value_len != NULL) len = *value_len;
    else len = strlen(value);

    char* value_dup = (char*)malloc(len + 1); // include null terminator
    if (value_dup == NULL)
        return;

    memcpy(value_dup, value, len);
    value_dup[len] = 0;
    
    map->arr[map->size].value = value_dup;
    map->arr[map->size].value_len = len;

    map->size += 1;
}

void map_remove(map* map, char* key) {
    if (map->arr == NULL)
        return;

    if (map->size == 1) {
        if (map->arr[0].key != NULL)
            free(map->arr[0].key);
        
        if (map->arr[0].value != NULL)
            free(map->arr[0].value);
        
        free(map->arr);

        map->arr = NULL;
        map->size = 0;
        return;
    }

    map_node* node_to_remove = map_find(map, key);
    if (node_to_remove == NULL)
        return;

    map_node* temp_arr = (map_node*)malloc(sizeof(map_node) * (map->size - 1));
    if (temp_arr == NULL)
        return;

    int j = 0;
    for (int i = 0; i < map->size; i++) {
        if (&map->arr[i] != node_to_remove) {
            temp_arr[j] = map->arr[i];
            j++;
        }
    }

    free(node_to_remove->key);

    if (node_to_remove->value != NULL)
        free(node_to_remove->value);
    
    // replace
    free(map->arr);
    map->arr = temp_arr;
    
    map->size -= 1;
}

map_node* map_find(map* map, char* key) {
    for (int i = 0; i < map->size; i++) {
        if (strcmp(map->arr[i].key, key) == 0)
            return &map->arr[i];
    }

    return NULL;
}

char* map_find_value(map* map, char* key) {
    map_node* node = map_find(map, key);
    if (node == NULL)
        return NULL;

    return node->value;
}

int map_get_pairs_size(map* map) {
    int size = 0;
    for (int i = 0; i < map->size; i++)
        size += strlen(map->arr[i].key) + map->arr[i].value_len;

    return size;
}

map map_dup(map* src_map) {
    map new_map = {0};
    if (src_map == NULL || src_map->arr == NULL || src_map->size == 0)
        return new_map; 
    
    int size = src_map->size * sizeof(map_node);
    map_node* dup = (map_node*)malloc(size);
    if (dup == NULL)
        return new_map;

    for (int i = 0; i < src_map->size; i++) {
        dup[i].key = strdup(src_map->arr[i].key);
        dup[i].value = strdup(src_map->arr[i].value);
        dup[i].value_len = src_map->arr[i].value_len;
    }

    new_map.arr = dup;
    new_map.size = src_map->size;
    return new_map;
}

void map_free(map* map) {
    for (int i = 0; i < map->size; i++) {
        if (map->arr[i].key != NULL)
            free(map->arr[i].key);

        if (map->arr[i].value != NULL)
            free(map->arr[i].value);
    }

    if (map->arr != NULL)
        free(map->arr);

    map->size = 0;
}