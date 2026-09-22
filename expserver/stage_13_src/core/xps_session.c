#include "xps_session.h"

xps_session_t *xps_session_create(xps_core_t *core, xps_connection_t *client){
    assert (core!=NULL && client!=NULL);

    xps_session_t* session = malloc(sizeof(xps_session_t));
    if (session == NULL) {
        logger(LOG_ERROR, "xps_session_create()", "malloc() failed for 'session'");
        return NULL;
    }

    session->client_source = xps_pipe_source_create(session, client_source_handler, client_source_close_handler);
    session->client_sink = xps_pipe_sink_create(session, client_sink_handler, client_sink_close_handler);
    session->upstream_source = xps_pipe_source_create(session, upstream_source_handler, upstream_source_close_handler);
    session->upstream_sink = xps_pipe_sink_create(session, upstream_sink_handler, upstream_sink_close_handler);
    session->file_sink = xps_pipe_sink_create(session, file_sink_handler, file_sink_close_handler);

    if (!(session->client_source && session->client_sink && session->upstream_source &&
            session->upstream_sink && session->file_sink)) {
        logger(LOG_ERROR, "xps_session_create()", "failed to create some sources/sinks");

        if (session->client_source) xps_pipe_source_destroy(session->client_source);
        if (session->client_sink) xps_pipe_sink_destroy(session -> client_sink);
        if (session->upstream_source) xps_pipe_source_destroy(session -> upstream_source);
        if (session->upstream_sink) xps_pipe_sink_destroy(session -> upstream_sink);
        if (session->file_sink) xps_pipe_sink_destroy(session -> file_sink);
        free(session);
        return NULL;
    }

    // Init values
    session->core = core;
    session->client = client;
    session->upstream = NULL;
    session->upstream_connected = false;
    session->upstream_error_res_set = false;
    session->upstream_write_bytes = 0;
    session->file = NULL;
    session->to_client_buff = NULL;
    session->from_client_buff = NULL;
    session->client_sink->ready = true;
    session->upstream_sink->ready = true;
    session->file_sink->ready = true;

    // Add current session to core->sessions
    vec_push(&(core->sessions), session);

    // Attach client
    if (xps_pipe_create(core, DEFAULT_PIPE_BUFF_THRESH, client->source, session->client_sink) ==
            NULL ||
        xps_pipe_create(core, DEFAULT_PIPE_BUFF_THRESH, session->client_source, client->sink) ==
            NULL) {
        logger(LOG_ERROR, "xps_session_create()", "failed to create client pipes");

        if (session->client_source) xps_pipe_source_destroy(session->client_source);
        if (session->client_sink) xps_pipe_sink_destroy(session -> client_sink);
        if (session->upstream_source) xps_pipe_source_destroy(session -> upstream_source);
        if (session->upstream_sink) xps_pipe_sink_destroy(session -> upstream_sink);
        if (session->file_sink) xps_pipe_sink_destroy(session -> file_sink);

        free(session);
        return NULL;
    }

    logger(LOG_DEBUG, "xps_session_create()", "created session");

    if (client->listener->port == 8001) {
        xps_connection_t *upstream = xps_upstream_create(core, "0.0.0.0", 3000);
        if (upstream == NULL) {
            logger(LOG_ERROR, "xps_session_create()", "xps_upstream_create() failed");
            perror("Error message");
            xps_session_destroy(session);
            return NULL;
        }
        session->upstream = upstream;
        xps_pipe_create(core, DEFAULT_PIPE_BUFF_THRESH, session->upstream_source,upstream->sink);
        xps_pipe_create(core, DEFAULT_PIPE_BUFF_THRESH, upstream->source, session->upstream_sink);
    }

    else if (client->listener->port == 8002) {
        int error;
        xps_file_t *file = xps_file_create(core, "../public/sample.txt", &error);
        if (file == NULL) {
            logger(LOG_ERROR, "xps_session_create()", "xps_file_create() failed");
            perror("Error message");
            xps_session_destroy(session);
            return NULL;
        }
        /* assign to the file member */
        xps_pipe_create(core, DEFAULT_PIPE_BUFF_THRESH, file->source, session->file_sink);
    }

    return session;
}
