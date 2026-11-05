#include <xps_gzip.h>

xps_gzip_t *xps_gzip_create(int level) {

    if (level < -1 || level > 9) {
        logger(LOG_ERROR, "xps_gzip_create()", "invalid compression level %d", level);
        return NULL;
    }

    xps_gzip_t *gzip = malloc(sizeof(xps_gzip_t));
    if(!gzip) {
        logger(LOG_ERROR, "xps_gzip_create()", "failed to allocate memory for xps_gzip_t");
        return NULL;
    }
    gzip->stream.zalloc = Z_NULL;
    gzip->stream.zfree = Z_NULL;
    gzip->stream.opaque = Z_NULL;

    gzip->source = xps_pipe_source_create((void *)gzip, gzip_source_handler, gzip_source_close_handler);
    if(!gzip->source) {
        logger(LOG_ERROR, "xps_gzip_create()", "failed to create pipe source");
        free(gzip);
        return NULL;
    }

    gzip->sink = xps_pipe_sink_create((void *)gzip, gzip_sink_handler, gzip_sink_close_handler);
    if(!gzip->sink) {
        logger(LOG_ERROR, "xps_gzip_create()", "failed to create pipe sink");
        xps_pipe_source_destroy(gzip->source);
        free(gzip);
        return NULL;
    }

    if (deflateInit2(&(gzip->stream), level, Z_DEFLATED, 16 + MAX_WBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        logger(LOG_ERROR, "xps_gzip_create()", "deflateInit2() failed");
        xps_pipe_source_destroy(gzip->source);
        xps_pipe_sink_destroy(gzip->sink);
        free(gzip);
        return NULL;
    }

    gzip->sink->ready = true;
    gzip->transfer_buff = NULL;
    gzip->total_in_len = 0;
    gzip->total_out_len = 0;

    return gzip;
}

void xps_gzip_destroy(xps_gzip_t *gzip) {
    assert(gzip);

    //cleanup
    deflateEnd(&(gzip->stream));
    xps_pipe_source_destroy(gzip->source);  
    xps_pipe_sink_destroy(gzip->sink);
    if (gzip->transfer_buff) xps_buffer_destroy(gzip->transfer_buff);

    free(gzip);
}

void gzip_compress(xps_gzip_t *gzip, bool flush){
    assert(gzip);
}
