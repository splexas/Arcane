#include "../include/http.h"
#include "../include/utilities.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

/* predefined compile time hash constants */

#define GET_HASH 0xb87ea25
#define POST_HASH 0x7c8a48eb
#define HEAD_HASH 0x7c85b8f7
#define PUT_HASH 0xb88127e
#define DELETE_HASH 0xab8947b8
#define CONNECT_HASH 0xe73478ef
#define OPTIONS_HASH 0x85ae7bb1
#define TRACE_HASH 0xe1d1ef4
#define PATCH_HASH 0xdcbbb55

int http_parse_request(char* raw_data, long int* max_headers_length, struct request* request_out) {
    /* read method, path and http ver */
    struct request request = {0};

    char* method = strtok(raw_data, " "); // first call loads the string into strtok
    if (method == NULL)
        return 1;
        
    switch (djb2(method)) {
        case GET_HASH: {
            request.method = GET;
            break;
        }
        case POST_HASH: {
            request.method = POST;
            break;
        }
        case HEAD_HASH: {
            request.method = HEAD;
            break;
        }
        case PUT_HASH: {
            request.method = PUT;
            break;
        }
        case DELETE_HASH: {
            request.method = DELETE;
            break;
        }
        case CONNECT_HASH: {
            request.method = CONNECT;
            break;
        }
        case OPTIONS_HASH: {
            request.method = OPTIONS;
            break;
        }
        case TRACE_HASH: {
            request.method = TRACE;
            break;
        }
        case PATCH_HASH: {
            request.method = PATCH;
            break;
        }
        default:
            return 1;
    }

    request.path = strtok(NULL, " ");
    if (request.path == NULL)
        return 1;

    request.http_ver = strtok(NULL, "\n");
    if (request.http_ver == NULL)
        return 1;

    request.http_ver[strlen(request.http_ver) - 1] = 0; // remove carriage return
    
    /* read all the headers */
    char* line = strtok(NULL, "\n");
    if (line == NULL)
        return 1;

    int headers_len = 0;

    while (*line != '\r') {
        char* save_ptr;
        
        char* key = strtok_r(line, " ", &save_ptr);
        if (key == NULL) {
            map_free(&request.headers);
            return 1;
        }

        char* value = strtok_r(NULL, "\r", &save_ptr);
        if (value == NULL) {
            map_free(&request.headers);
            return 1;
        }

        key[strlen(key) - 1] = 0; // remove ':'

        /* check key/value lengths */
        headers_len += strlen(key) + strlen(value);
        if (headers_len > *max_headers_length) {
            map_free(&request.headers);
            return 2;
        }

        for (int i = 0; key[i]; i++)
            key[i] = tolower(key[i]);

        map_push(&request.headers, key, value, NULL);

        line = strtok(NULL, "\n");
        if (line == NULL) {
            map_free(&request.headers);
            return 1;
        }
    }

    char* data = strtok(NULL, "\0");
    if (data != NULL) request_out->data = data;

    *request_out = request;
    return 0;
}


char* http_make_status_line(int* status_code, char* http_version, map* status_messages) {
    // HTTP/1.1 200 OK\r\n

    char code[4];
    code[3] = 0; // null terminate
    sprintf(code, "%d", *status_code);

    char* status_message = map_find_value(status_messages, code);
    if (status_message == NULL)
        return NULL;

    int status_line_size = strlen(http_version) + strlen(status_message) + strlen(code) + 4;
    char* status_line = (char*)calloc(status_line_size + 1, 1); // + null terminator
    if (status_line == NULL)
        return NULL;

    strcat(status_line, http_version);
    strcat(status_line, " ");
    strcat(status_line, code);
    strcat(status_line, " ");
    strcat(status_line, status_message);
    strcat(status_line, "\r\n");

    return status_line;
}

char* http_make_raw_headers(map* headers) {
    // key: vaLue\r\n

    int size = map_get_pairs_size(headers) + headers->size * 4 + 2;
    char* raw_headers = (char*)calloc(size + 1, 1); // + null terminator
    if (raw_headers == NULL)
        return NULL;
    
    for (int i = 0; i < headers->size; i++) {
        strcat(raw_headers, headers->arr[i].key);
        strcat(raw_headers, ": ");
        strcat(raw_headers, headers->arr[i].value);
        strcat(raw_headers, "\r\n");
    }

    strcat(raw_headers, "\r\n"); // finish headers
    return raw_headers;
}