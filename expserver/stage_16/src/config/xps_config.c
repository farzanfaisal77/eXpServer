#include "xps_config.h"

xps_config_t *xps_config_create(const char *config_path){
    assert(config_path);

    xps_config_t *config = malloc(sizeof(xps_config_t));
    if (!config) {
        logger(LOG_ERROR, "xps_config_create()", "Memory allocation failed");
        return NULL;
    }

    JSON_Value *config_json = json_parse_file(config_path);
    if (!config_json){
        logger(LOG_ERROR, "xps_config_create()", "JSON parse file failed");
        free(config);
        return NULL;
    }

    config->config_path = config_path;
    config->_config_json = config_json;
    vec_init(&(config->servers));
    vec_init(&(config->_all_listeners));

    JSON_Object *root_object = json_value_get_object(config_json);
    if (!root_object){
        logger(LOG_ERROR, "xps_config_create()", "JSON root object not found");
        free(config);
        return NULL;
    }

    config->server_name = json_object_get_string(root_object, "server_name");
    JSON_Array *servers = json_object_get_array(root_object, "servers");

    for (int i=0; i < json_array_get_count(servers); i++){
        xps_config_server_t *server = malloc(sizeof(xps_config_server_t));
        if (!server){
            logger(LOG_ERROR, "xps_config_create()", "Memory allocation failed");
            free(config);
            return NULL;
        }
        parse_server(json_array_get_object(servers, i), server);
        vec_push(&(config->servers), server);
    }
    parse_all_listeners(config);

    return config;
}

void xps_config_destroy(xps_config_t *config) {
    assert(config);

    vec_void_t *servers = &config->servers;
    for(int i = 0; i < servers->length; i++) {
        xps_config_server_t *server = servers->data[i];
        if(server) {
            vec_void_t *listeners = &server->listeners;
            for(int j = 0; j < listeners->length; j++) {
                xps_config_listener_t *listener = listeners->data[j];
                if(listener) free(listener);
            }
            vec_deinit(&server->listeners);
            vec_deinit(&server->hostnames);
            vec_void_t *routes = &server->routes;
            for(int j = 0; j < routes->length; j++) {
                xps_config_route_t *route = routes->data[j];
                if(route) {
                    vec_deinit(&route->index);
                    vec_deinit(&route->upstreams);
                    free(route);
                }
            }
            vec_deinit(&server->routes);
            free(server);
        }
    }
    vec_deinit(&(config->servers));
}


xps_config_lookup_t *xps_config_lookup(xps_config_t *config, xps_http_req_t *http_req, xps_connection_t *client, int *error) {

    assert(config && http_req && client);

    *error = E_FAIL;

    char* host = http_req->host;
    char* pathname = http_req->pathname;
    const char* accept_encoding = xps_http_get_header(&http_req->headers, "Accept-Encoding");

    // Step 1: Find matching server block
    int target_server_index = -1;
    vec_void_t *servers = &config->servers;

    for (int i=0; i < servers->length; i++) {
        xps_config_server_t *server = servers->data[i];
        // Check if client listener is present in server
        vec_void_t *listeners = &server->listeners;
        int has_matching_listener = false;
        for (int j = 0; j < listeners->length; j++) {
            xps_config_listener_t *listener = listeners->data[j];
            if (strcmp(listener->host, client->listener->host) == 0 && listener->port == client->listener->port) {
                has_matching_listener = true;
                break;
            }
        }
        if (!has_matching_listener) continue;
        /* Check if host header matches any hostname*/
        vec_void_t *hostnames = &server->hostnames;
        int has_matching_hostname = false;
        if(hostnames->length == 0) {
            has_matching_hostname = true;
        }
        else {
            for (int j = 0; j < hostnames->length; j++) {
                const char *hostname = hostnames->data[j];
                if (strcmp(hostname, host) == 0) {
                    has_matching_hostname = true;
                    break;
                }
            }
        }
        // this will only be true when the server has both matching listener and hostname (hostname in the listeners list)
        if (has_matching_hostname) {
            target_server_index = i;
            break;
        }
    }

    if(target_server_index == -1) {
        logger(LOG_ERROR, "xps_config_lookup()", "No matching server found for host: %s, listener: %s:%u", host, client->listener->host, client->listener->port);
        *error = E_NOTFOUND;  // No matching server found - 404
        return NULL;
    }

    xps_config_server_t *server = config->servers.data[target_server_index];

    /*Find matching route block*/
    // Route matching uses prefix matching with longest-match-first strategy.
    // This is important because:
    // - For file serving routes (e.g., "/"), we need to match any path under it
    //   (e.g., "/index.html", "/css/style.css" should all match route "/")
    // - For specific routes (e.g., "/api"), we want them to take precedence over "/"
    //
    // Example: If we have routes "/" and "/api"
    // - Request "/index.html" matches "/" only → serves file from "/"
    // - Request "/api/users" matches both "/" and "/api" → use "/api" (longest match)

    xps_config_route_t *route = NULL;
    size_t best_match_len = 0;  // Track the longest matching route path

    for (int i = 0; i < server->routes.length; i++) {
        xps_config_route_t *current_route = server->routes.data[i];
        size_t route_path_len = strlen(current_route->req_path);

        // Check if this route's path is a prefix of the request path
        if (str_starts_with(pathname, current_route->req_path)) {
            // If this is a longer match than we've found so far, use it
            if (route_path_len > best_match_len) {
                best_match_len = route_path_len;
                route = current_route;
            }
        }
    }

    if (route == NULL) {
        *error = E_NOTFOUND;  // No matching route found - 404
        return NULL;
    }
    /* Init values of lookup*/
    xps_config_lookup_t *lookup = malloc(sizeof(xps_config_lookup_t));
    if (lookup == NULL) {
        logger(LOG_ERROR, "xps_config_lookup()", "Failed to allocate memory for lookup");
        *error = E_FAIL;
        return NULL;
    }
    vec_init(&lookup->ip_whitelist);
    vec_init(&lookup->ip_blacklist);
    lookup->file_path = NULL;
    lookup->dir_path = NULL;
    lookup->upstream = NULL;
    lookup->http_status_code = 0;
    lookup->redirect_url = NULL;

    if(strcmp(route->type, "file_serve") == 0) {
        lookup->type = REQ_FILE_SERVE;
    }
    else if(strcmp(route->type, "reverse_proxy") == 0) {
        lookup->type = REQ_REVERSE_PROXY;
    }
    else if(strcmp(route->type, "redirect") == 0) {
        lookup->type = REQ_REDIRECT;
    }
    else if(strcmp(route->type, "metrics") == 0) {
        lookup->type = REQ_METRICS;
    }
    else {
        lookup->type = REQ_INVALID;
    }

    if (lookup->type == REQ_FILE_SERVE) {
        char *resource_path = path_join(route->dir_path, pathname);

        if (resource_path == NULL) {
            logger(LOG_WARNING, "xps_config_lookup()", "Failed to construct resource path");
            *error = E_FAIL;
            free(lookup);
            return NULL;
        }

        if (!is_abs_path(resource_path)) {
            /* we require abosulte path so we need to see
            if the current path is absolute or not */
            /*fill here*/
            char *abs_path = realpath(resource_path, NULL);
            free(resource_path);
            resource_path = abs_path;
        }
        if(resource_path == NULL) {
            logger(LOG_WARNING, "xps_config_lookup()", "Resource not found: %s", pathname);
            *error = E_NOTFOUND;
            free(lookup);
            return NULL;
        }
        // is file
        if (is_file(resource_path)) {
            lookup->file_path = resource_path;
        } 
        else if (is_dir(resource_path)) { // is directory
            /* If request is for a directory, serve the index file (e.g. index.html)
            * instead of showing the directory listing. */
            bool index_file_found = false;
            for (int i = 0; i < route->index.length; i++) {
                char *index_file = path_join(resource_path, route->index.data[i]);
                /*fill here*/
                if(is_file(index_file)) {
                    lookup->file_path = index_file;
                    index_file_found = true;
                    break;
                } 
                else free(index_file);   
            }
            if (!index_file_found) {
                /*no index file so free the resource_path*/
                *error = E_NOTFOUND;
                free(lookup);
                free(resource_path);
                return NULL;
            }
        } 
        else {
            /*no matching type so free resource_path*/
            free(resource_path);
            *error = E_NOTFOUND;
            free(lookup);
            return NULL;
        }
        *error = OK;
        return lookup;
    }
    if (lookup->type == REQ_REVERSE_PROXY) {
        // For reverse proxy, we can just take the first upstream from the list of upstreams
        lookup->upstream = route->upstreams.data[0];
        *error = OK;
        return lookup;
    }

    if (lookup->type == REQ_REDIRECT) {
        // For redirect, we can just take the http_status_code and redirect_url from the route
        lookup->http_status_code = route->http_status_code;
        lookup->redirect_url = route->redirect_url;
        *error = OK;
        return lookup;
    }

    free(lookup);
    *error = E_FAIL;
    return NULL;
}
