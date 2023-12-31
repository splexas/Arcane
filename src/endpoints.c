#include "../include/endpoints.h"
#include "../include/arcane.h" // for wrappers

void signin_endpoint(struct webclient* client, struct request* request, struct response* response_out) {
    arcane_set_status_code(200, response_out);
    arcane_set_data("test!", response_out);
    arcane_set_header("test", "test", response_out);
}

void signup_endpoint(struct webclient* client, struct request* request, struct response* response_out) {
    arcane_set_status_code(200, response_out);
}