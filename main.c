#include "include/arcane.h"
#include "include/endpoints.h"
#include "include/utilities.h"
#include "include/logger.h"

int main() {
    struct dynamic_route dynamic_routes[] = {
        {POST, "/signin", signin_endpoint},
        {POST, "/signup", signup_endpoint}
    };

    int dynamic_routes_len = sizeof(dynamic_routes) / sizeof(struct dynamic_route);

    if (arcane_initialize(
        "arcane.conf", 
        dynamic_routes, 
        &dynamic_routes_len
    ) != 0) {
        ARCANE_LOG(NULL, ARC_INFO, arc_false, "failed to init the server.\n");
        return 1;
    }

    ARCANE_LOG(NULL, ARC_INFO, arc_false, "initialized the server!\n");

    arcane_run();
    arcane_deinitialize();
    return 0;
}