#include "../include/arcane.h"
#include "../include/utilities.h"
#include "../include/logger.h"
#include "../include/updater.h"

#ifdef _WIN32
#include <winsock2.h>
#else
// linux headers
#endif

#include <stdio.h>
#include <pthread.h>

int server_fd;
struct config config = {0};
/****/
map default_headers = {0};
map status_messages = {0};
/****/
map static_content_route_map = {0};
map static_content = {0};
map static_assets = {0};
/***/

struct dynamic_route* dynamic_routes;
int* dynamic_routes_len;


int arcane_init_server(char* ip, long int* port) {
    #ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2,2), &wsa_data) != 0) {
        ARCANE_LOG(NULL, ARC_ERROR, arc_false, "[arcane_initialize] Failed to WSA startup.\n");
        return 1;
    }
    #endif

    server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd == -1) {
        ARCANE_LOG(NULL, ARC_ERROR, arc_false, "[arcane_initialize] Failed to initialize the server socket file descriptor.\n");
        #ifdef _WIN32
        WSACleanup();
        #endif
        return 1;
    }

    struct sockaddr_in addr_in;
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(*port);
    #ifdef _WIN32
    addr_in.sin_addr.S_un.S_addr = inet_addr(ip);
    #else
    addr_in.sin_addr.s_addr = inet_addr(ip);
    #endif

    struct sockaddr addr = *(struct sockaddr*)&addr_in;
    if (bind(server_fd, &addr, sizeof(addr)) == -1) {
        ARCANE_LOG(config.log_file, ARC_ERROR, arc_false, "[arcane_initialize] Failed to bind the server socket file descriptor.\n");
        #ifdef _WIN32
        WSACleanup();
        #endif
        return 1;
    }

    const char enable = 1;
    #ifdef _WIN32
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable))) {
        ARCANE_LOG(config.log_file, ARC_ERROR, arc_false, "[arcane_initialize] Failed to set reusable socket property on server socket file descriptor.\n");
        WSACleanup();
        return 1;
    }
    #else
    if (setsockopt(this->server_fd, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(enable))) {
        ARCANE_LOG(config.log_file, ARC_ERROR, arc_false, "[arcane_initialize] Failed to set reusable socket property on server socket file descriptor.\n");
        return 1;
    }
    #endif

    if (listen(server_fd, 255) == -1) {
        ARCANE_LOG(config.log_file, ARC_ERROR, arc_false, "[arcane_initialize] Failed to listen for connections on server socket file descriptor.\n");
        #ifdef _WIN32
        WSACleanup();
        #endif
        return 1;
    }

    return 0;
}

int arcane_initialize(char* config_path, struct dynamic_route* _dynamic_routes, int* _dynamic_routes_len) {
    /********** parse the config **********/

    if (utils_parse_config(config_path, &config) != ARCANE_OK) {
        ARCANE_LOG(NULL, ARC_ERROR, arc_false, "[arcane_initialize] Failed to parse the config file.\n");
        return 1;
    }

    /********** init the server **********/

    if (arcane_init_server(config.server_ip, &config.server_port) != ARCANE_OK) {
        ARCANE_LOG(NULL, ARC_ERROR, arc_false, "[arcane_initialize] Server failed to initialize\n");
        goto failure;
    }

    /********** parsing static content routes config **********/

    map static_content_route_map = {0};

    if (utils_parse_routes(config.routes_json, &static_content_route_map) != ARCANE_OK) {
        ARCANE_LOG(config.log_file, ARC_ERROR, arc_false, "[arcane_initialize] Failed to parse the static content routes.\n");
        goto failure;
    }

    /********** config **********/

    if (config.server_name != NULL) {
        if (config.show_server_name == cfg_true)
            map_push(&default_headers, "server", config.server_name, NULL);
    }

    map_push(&default_headers, "connection", "close", NULL);

    #ifdef DEBUG
    for (int i = 0; i < default_headers.size; i++)
        ARCANE_LOG(config.log_file, ARC_DEBUG, arc_false, "[arcane_initialize] Loaded default header: %s: %s\n", default_headers.arr[i].key, default_headers.arr[i].value);
    #endif

    for (int i = 0; i < static_content_route_map.size; i++) {
        ARCANE_LOG(config.log_file, ARC_INFO, arc_false, "[arcane_initialize] Loaded static content route: %s: %s\n", static_content_route_map.arr[i].key, static_content_route_map.arr[i].value);
    }

    /********** load webpages **********/

    static_content = utils_load_content_map(&static_content_route_map);
    map_free(&static_content_route_map);

    if (static_content.arr == NULL) {
        ARCANE_LOG(config.log_file, ARC_ERROR, arc_false, "[arcane_initialize] Failed to load the webpages\n");
        map_free(&default_headers);
        goto failure;
    }

    /********** load assets  **********/

    int static_asset_files_len = 0;
    char** static_asset_files = utils_get_dir_files(config.assets_dir, &static_asset_files_len); // it can be NULL

    if (static_asset_files != NULL) {
        static_assets = utils_load_content(static_asset_files, static_asset_files_len);

        for (int i = 0; i > static_asset_files_len; i++)
            free(static_asset_files[i]);

        free(static_asset_files);

        if (static_assets.arr == NULL) {
            ARCANE_LOG(config.log_file, ARC_ERROR, arc_false, "[arcane_initialize] Failed to load the assets\n");
            map_free(&default_headers);
            goto failure;
        }
    }

    /********** execute static content/assets updater thread **********/

    struct updater_args* upd_args = (struct updater_args*)malloc(sizeof(struct updater_args));
    memset(upd_args, 0, sizeof(struct updater_args));

    upd_args->static_content = &static_content;
    upd_args->static_content_routes_file = config.routes_json;

    if (static_assets.arr != NULL) {
        upd_args->static_assets_dir = config.assets_dir;
        upd_args->static_assets = &static_assets;
    }

    pthread_t thread_id;
    pthread_create(&thread_id, NULL, (void*)&updater_update, (void*)upd_args);
    pthread_detach(thread_id);

    /********** dynamic routes **********/

    dynamic_routes = _dynamic_routes;
    dynamic_routes_len = _dynamic_routes_len;

    /********** add most useful http request status messages **********/

    map_push(&status_messages, "200", "OK", NULL);
    map_push(&status_messages, "400", "Bad Request", NULL);
    map_push(&status_messages, "401", "Unauthorized", NULL);
    map_push(&status_messages, "403", "Forbidden", NULL);
    map_push(&status_messages, "404", "Not Found", NULL);
    map_push(&status_messages, "413", "Entity Too Large", NULL);
    map_push(&status_messages, "500", "Internal Server Error", NULL);
    map_push(&status_messages, "501", "Not Implemented", NULL);
    map_push(&status_messages, "505", "HTTP Version Not Supported", NULL);
    return 0;

failure:
    arcane_free_config();
    return 1;
}

void arcane_accept_clients() {
    while (1) {
        struct webclient* client = (struct webclient*)malloc(sizeof(struct webclient));
        if (client == NULL)
            continue;
        
        struct sockaddr addr;
        int addr_len = sizeof(addr);

        client->fd = accept(server_fd, &addr, &addr_len);
        if (client->fd == -1) {
            free(client);
            continue;
        }

        /* add recv timeout properties */
        #ifdef _WIN32
        if (setsockopt(client->fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&config.general_timeout, sizeof(config.general_timeout)) == -1) {
            #ifdef DEBUG
            ARCANE_LOG(config.log_file, ARC_DEBUG, arc_false, "[arcane_accept_clients] Failed to set the general timeout (recv) for client.\n");
            #endif
            
            /* close the client socket */
            #ifdef _WIN32
            closesocket(client->fd);
            #else
            close(client->fd);
            #endif
            
            free(client);
            continue;
        }
        #else
        struct timeval timeout = {0};
        timeout.tv_usec = config.general_timeout * 1000; // microseconds
        if (setsockopt(client->fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) == -1) {
            #ifdef DEBUG
            ARCANE_LOG(config.log_file, ARC_DEBUG, arc_false, "[arcane_accept_clients] Failed to set the general timeout (recv) for client.\n");
            #endif

            /* close the client socket */
            #ifdef _WIN32
            closesocket(client->fd);
            #else
            close(client->fd);
            #endif

            free(client);
            continue;
        }
        #endif

        client->ip = inet_ntoa(((struct sockaddr_in*)(&addr))->sin_addr);
        if (client->ip == NULL) {
            /* close the client socket */
            #ifdef _WIN32
            closesocket(client->fd);
            #else
            close(client->fd);
            #endif

            free(client);
            continue;
        }

        client->ip = strdup(client->ip);

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, (void*)&arcane_handle_client, (void*)client);
        pthread_detach(thread_id);

        /* now `arcane_handle_client` is responsible for freeing `client` and `client->ip`. */
    }
}

void arcane_handle_client(void* p_client) {
    struct webclient* client = (struct webclient*)p_client;
    ARCANE_LOG(config.log_file, ARC_INFO, arc_false, "[arcane_handle_client] Handling %s\n", client->ip);

    char* received_data = arcane_receive_data(client);
    if (received_data != NULL) {
        // proceed to process the request further

        #ifdef DEBUG
        ARCANE_LOG(NULL, ARC_DEBUG, arc_false, "Data received: %s\n", received_data);
        #endif

        struct request parsed_request = {0};

        /* todo: provide some headers even if the http request is malformed, such as: server, date and so on? */
        int parse_status = http_parse_request(received_data, &config.max_headers_length, &parsed_request);
        if (parse_status == 0) {
            ARCANE_LOG(config.log_file, ARC_INFO, arc_false, "Method: %d\n", parsed_request.method);
            ARCANE_LOG(config.log_file, ARC_INFO, arc_false, "Path: %s\n", parsed_request.path);
            ARCANE_LOG(config.log_file, ARC_INFO, arc_false, "User-Agent: %s\n", map_find_value(&parsed_request.headers, "user-agent"));
            
            struct response response = {0};
            response.headers = map_dup(&default_headers);
            // 0 in response.status_code means the request wasn't handled yet

            char is_dynamic = 0;

            /* prioritize dynamic routes */
            for (int i = 0; i < *dynamic_routes_len; i++) {
                if (dynamic_routes[i].method == parsed_request.method && strcmp(dynamic_routes[i].path, parsed_request.path) == 0) {
                    dynamic_routes[i].callback(client, &parsed_request, &response);
                    is_dynamic = 1; // because of strdup
                    break;
                }
            }

            /* provide static content */
            if (response.status_code == 0) {
                response.status_code = 404;
                if (parsed_request.method == GET) {
                    // check if asset


                    // provide webpage

                    response.data = map_find_value(&static_content, parsed_request.path);
                    if (response.data != NULL) {
                        response.status_code = 200;
                        map_push(&response.headers, "content-type", "text/html; charset=utf-8", NULL);
                    } else {
                        // render 404 not found page
                    }
                }

                // handle other methods
            }

            /* add the date header */
            char timezone[256];
            ARCANE_GET_TIMESTAMP("%a, %d %b %Y %H:%M:%S GMT", timezone, sizeof(timezone));
            map_push(&response.headers, "date", timezone, NULL);

            /* gzip compress */
            char is_data_compressed = 0;
            int response_data_len = 0;

            if (response.data != NULL) {
                char* accept_encoding = map_find_value(&parsed_request.headers, "accept-encoding");
                // check if the request supports gzip compression
                if (accept_encoding != NULL && strstr(accept_encoding, "gzip") != NULL) {
                    char* compressed_data =  utils_gzip_compress(response.data, &response_data_len);
                    if (compressed_data != NULL) {
                        if (is_dynamic == 1) free(response.data);
                        response.data = compressed_data;

                        /* add gzip content encoding type */
                        map_push(&response.headers, "content-encoding", "gzip", NULL);
                        is_data_compressed = 1;

                    } else
                        response_data_len = strlen(response.data);
                } else
                    response_data_len = strlen(response.data);

                /* add content-length header */
                char content_length[10];
                content_length[9] = 0; // null terminate
                sprintf(content_length, "%d", response_data_len);
                map_push(&response.headers, "content-length", content_length, NULL);
            }
            
            if (arcane_send_status(client, response.status_code) != ARCANE_OK) {
                ARCANE_LOG(config.log_file, ARC_ERROR, arc_false, "Failed to send HTTP status line.\n");
                goto client_cleanup;
            }

            #ifdef DEBUG
            for (int i = 0; i < response.headers.size; i++)
                ARCANE_LOG(config.log_file, ARC_DEBUG, arc_false, "[arcane_handle_client] Sending headers: %s: %s\n", response.headers.arr[i].key, response.headers.arr[i].value);
            #endif

            if (response.headers.arr != NULL && arcane_send_headers(client, &response.headers) != ARCANE_OK) {
                ARCANE_LOG(config.log_file, ARC_DEBUG, arc_false, "Failed to send HTTP headers\n");
                goto client_cleanup;
            }

            #ifdef DEBUG
            ARCANE_LOG(config.log_file, ARC_DEBUG, arc_false, "[arcane_handle_client] response_data_len: %d\n", response_data_len);
            #endif

            if (response.data != NULL && arcane_send_data(client, response.data, &response_data_len) != ARCANE_OK)
                ARCANE_LOG(config.log_file, ARC_DEBUG, arc_false, "Failed to send HTTP data\n");

        client_cleanup:
            if (is_data_compressed == 1 || is_dynamic == 1) free(response.data);
            map_free(&response.headers);
            map_free(&parsed_request.headers);

        } else if (parse_status == 1) {
            arcane_send_status(client, 400);
        } else if (parse_status == 2) {
            arcane_send_status(client, 413);
        }
        
    }

    /* cleanup --- */
    free(received_data);

    #ifdef _WIN32
    closesocket(client->fd);
    #else
    close(client->fd);
    #endif

    free(client->ip);
    free(client);
}

char* arcane_receive_data(struct webclient* client) {
    int buf_size = 4096;
    char* buf = (char*)malloc(buf_size);
    if (buf == NULL)
        return NULL;

    int bytes_received = recv(client->fd, buf, buf_size, 0);
    if (bytes_received <= 0) {
        free(buf);
        return NULL;
    }

    buf[bytes_received] = 0; // null terminate
    return buf;
}

int arcane_send_data(struct webclient* client, char* raw_data, int* raw_data_len) {
    int len;
    if (raw_data_len != NULL) len = *raw_data_len;
    else len = strlen(raw_data);

    int bytes_sent = send(client->fd, raw_data, len, 0);
    if (bytes_sent <= 0)
        return 1;
    
    return 0;
}

int arcane_send_status(struct webclient* client, int status_code) {
    char* status_line = http_make_status_line(&status_code, "HTTP/1.1", &status_messages);
    if (status_line == NULL)
        return 1;
    
    #ifdef DEBUG
    ARCANE_LOG(config.log_file, ARC_DEBUG, arc_false, "[arcane_send_status] http_make_status_line result: %s\n", status_line);
    #endif

    int server_status = arcane_send_data(client, status_line, NULL);
    free(status_line);
    return server_status;
}

int arcane_send_headers(struct webclient* client, map* headers) {
    char* raw_headers = http_make_raw_headers(headers);
    if (raw_headers == NULL)
        return 1;
    
    #ifdef DEBUG
    ARCANE_LOG(config.log_file, ARC_DEBUG, arc_false, "[arcane_send_headers] raw_headers result: %s\n", raw_headers);
    #endif
    
    int server_status = arcane_send_data(client, raw_headers, NULL);
    free(raw_headers);
    return server_status;
}

void arcane_run() {
    arcane_accept_clients();
}

void arcane_free_config() {
    free(config.server_ip);
    free(config.server_name);
    free(config.routes_json);
    free(config.assets_dir);
    free(config.log_file);
}

void arcane_free_maps() {
    map_free(&default_headers);
    map_free(&status_messages);
    map_free(&static_content);
    map_free(&static_assets);
}

int arcane_deinitialize() {
    #ifdef _WIN32
    closesocket(server_fd);
    WSACleanup();
    #else
    close(server_fd);
    #endif

    arcane_free_config();
    arcane_free_maps();
    return 0;
}


void arcane_set_status_code(int status_code, struct response* response_out) {
    response_out->status_code = status_code;
}

void arcane_set_header(char* key, char* value, struct response* response_out) {
    map_push(&response_out->headers, key, value, NULL);
}

void arcane_set_data(char* data, struct response* response_out) {
    response_out->data = strdup(data);
}