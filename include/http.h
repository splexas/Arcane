#ifndef HTTP_H
#define HTTP_H

#include "map.h"

enum http_method {
    GET,
    POST,
    HEAD,
    PUT,
    DELETE,
    CONNECT,
    OPTIONS,
    TRACE,
    PATCH
};

struct webclient {
    int fd;
    char* ip;
};

struct request {
    char* http_ver;
    char* path;
    enum http_method method;
    map headers;
    char* data;
};

struct response {
    int status_code;
    map headers;
    char* data;
};

/* As long as `raw_data` is valid, the request struct will remain with valid data. */
/* Returns 1 if the request was incorrect or malformed. */
/* Returns 2 if the `max_headers_length` was exceeded. */
int http_parse_request(char* raw_data, long int* max_headers_length, struct request* request_out);

/* Returns a dynamically allocated null-terminated char array. */
char* http_make_status_line(int* status_code, char* http_version, map* status_messages);

/* Returns a dynamically allocated null-terminated char array. */
char* http_make_raw_headers(map* headers);

#endif