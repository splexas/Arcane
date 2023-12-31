#ifndef ARCANE_H
#define ARCANE_H

#include "http.h"
#include "map.h"

enum arcane_values {
    ARCANE_OK,
    ARCANE_ERROR
};

struct dynamic_route {
    enum http_method method;
    char* path;
    void (*callback)(struct webclient* client, struct request* request, struct response* response_out);
};

/* --- Extern most useful variables --- */

extern struct config config;

/* --- Server --- */

int arcane_init_server(char* ip, long int* port);
int arcane_initialize(char* config_path, struct dynamic_route* _dynamic_routes, int* _dynamic_routes_len);

void arcane_accept_clients();
void arcane_handle_client(void* p_client); // thread

/* returns a dynamically allocated null-terminated char array buffer. */
char* arcane_receive_data(struct webclient* client);

/* `raw_data_len` is optional. */
int arcane_send_data(struct webclient* client, char* raw_data, int* raw_data_len);
int arcane_send_status(struct webclient* client, int status_code);
int arcane_send_headers(struct webclient* client, map* headers);

void arcane_run();

/* --- Cleanup functions --- */

void arcane_free_config();
void arcane_free_maps();
int arcane_deinitialize();

/* --- Wrappers for dynamic route functions --- */

void arcane_set_status_code(int status_code, struct response* response_out);
void arcane_set_header(char* key, char* value, struct response* response_out);
void arcane_set_data(char* data, struct response* response_out);

#endif