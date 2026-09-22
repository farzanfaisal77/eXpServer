#include "xps_session.h"

xps_session_t *xps_session_create(xps_core_t *core, xps_connection_t *client){
    assert (core!=NULL && client!=NULL);

    xps_session_t* session = malloc(sizeof(xps_session_t));
    
}