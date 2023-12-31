#ifndef ENDPOINTS_H
#define ENDPOINTS_H

#include "http.h"

void signin_endpoint(struct webclient* client, struct request* request, struct response* response_out);
void signup_endpoint(struct webclient* client, struct request* request, struct response* response_out);

#endif