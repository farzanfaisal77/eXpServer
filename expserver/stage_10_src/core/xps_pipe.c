#include "xps_pipe.h"

xps_pipe_t *xps_pipe_create(xps_core_t *core, size_t buff_thresh, xps_pipe_source_t *source,
                            xps_pipe_sink_t *sink) {
    assert(core != NULL);
    assert(buff_thresh > 0);
    assert(source != NULL);
    assert(sink != NULL);

    // Alloc memory for pipe instance
    xps_pipe_t *pipe = malloc(sizeof(xps_pipe_t*));
    if (pipe == NULL) {
    logger(LOG_ERROR, "xps_pipe_create()", "malloc() failed for 'pipe'");
    return NULL;
    }

    /*Create buff_list instance*/
    xps_buffer_list_t*  buff_list = xps_buffer_list_create();

    // Init values
    pipe->core = core;
    pipe->source = NULL;
    pipe->sink = NULL;
    pipe->buff_list = buff_list;
    pipe->buff_thresh = buff_thresh;

    /* Add pipe to 'pipes' list of core (see core module below)*/
    vec_push(&core->pipes, pipe);

    /*Attach source and sink to pipe*/
    pipe->source = source;
    pipe->sink = sink;
    /*Make both source and sink of pipe active*/

    logger(LOG_DEBUG, "xps_pipe_create()", "created pipe");

    return pipe;
}

void xps_pipe_destroy(xps_pipe_t *pipe) {
    assert(pipe != NULL);

    /*Set NULL in 'pipes' list of core and increment n_null_pipes*/
    for(int i=0; i<pipe->core->pipes.length; i++){
        if(pipe = pipe->core->pipes.data[i]){
            pipe->core->pipes.data[i] = NULL;
            pipe->core->n_null_pipes++;
        }
    }
    
    /*Destroy the buff_list of pipe*/
    xps_buffer_destroy(&(pipe->buff_list));
    /*Free the pipe*/
    free(pipe);
    logger(LOG_DEBUG, "xps_pipe_destroy()", "destroyed pipe");
}