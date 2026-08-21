#include "../xps.h"
#include "xps_loop.h"


bool valid_event(xps_loop_t *loop, loop_event_t *event) {
    for (int j = 0; j < loop->events.length; j++) {
        if (loop->events.data[j] == event)
            return 1;
    }
    return 0;
}

loop_event_t *loop_event_create(u_int fd, void *ptr, xps_handler_t read_cb) {
  assert(ptr != NULL);

  loop_event_t *event = malloc(sizeof(loop_event_t));
  if (event == NULL) {
    logger(LOG_ERROR, "event_create()", "malloc() failed for 'event'");
    return NULL;
  }

  /* set fd, ptr, read_cb fields of event */
  event->fd = fd;
  event->ptr = ptr;
  event->read_cb = read_cb;

  logger(LOG_DEBUG, "event_create()", "created event");

  return event;
}

void loop_event_destroy(loop_event_t *event) {
  assert(event != NULL);

  free(event);

  logger(LOG_DEBUG, "event_destroy()", "destroyed event");
}

/**
 * Creates a new event loop instance associated with the given core.
 *
 * This function creates an epoll file descriptor, allocates memory for the xps_loop instance,
 * and initializes its values.
 *
 * @param core : The core instance to which the loop belongs
 * @return A pointer to the newly created loop instance, or NULL on failure.
 */
xps_loop_t *xps_loop_create(xps_core_t *core) {
  assert(core != NULL);

  /* fill this */
  int epoll_fd = epoll_create1(0);
    if(epoll_fd==-1){
        logger(LOG_ERROR, "xps_loop_create()", "epoll_create1() failed");
        return NULL;
    }


  xps_loop_t* loop = malloc(sizeof(xps_loop_t));
  if(loop==NULL){
    logger(LOG_ERROR, "xps_loop_create()", "malloc() failed");
    close(epoll_fd);
    return NULL;
  }

  loop->epoll_fd = epoll_fd;
  loop->core = core;
  loop->n_null_events = 0;  
  vec_init(&loop->events);

  logger(LOG_DEBUG, "xps_loop_create()", "created event loop instance");

  return loop;
}

/**
 * Destroys the given loop instance and releases associated resources.
 *
 * This function destroys all loop_event_t instances present in loop->events list,
 * closes the epoll file descriptor and releases memory allocated for the loop instance,
 *
 * @param loop The loop instance to be destroyed.
 */
void xps_loop_destroy(xps_loop_t *loop) {
  assert(loop != NULL);

  close(loop->epoll_fd);
  for(int i=0; i<loop->events.length; i++){
    loop_event_t *ev= loop->events.data[i];
    if(ev != NULL) loop_event_destroy(ev);
  }
  vec_deinit(&loop->events);

  free(loop);
  loop=NULL;
  return;
}

/**
 * Attaches a FD to be monitored using epoll
 *
 * The function creates an instance of loop_event_t and attaches it to epoll.
 * Add the pointer to loop_event_t to the events list in loop
 *
 * @param loop : loop to which FD should be attached
 * @param fd : FD to be attached to epoll
 * @param event_flags : epoll event flags
 * @param ptr : Pointer to instance of xps_listener_t or xps_connection_t
 * @param read_cb : Callback function to be called on a read event
 * @return : OK on success and E_FAIL on error
 */
int xps_loop_attach(xps_loop_t *loop, u_int fd, int event_flags, void *ptr, xps_handler_t read_cb, xps_handler_t write_cb, xps_handler_t close_cb){
  assert(loop != NULL);
  assert(ptr != NULL);
  assert(read_cb != NULL);
  /* assert(write_cb != NULL);
  assert(close_cb != NULL); */

  loop_event_t * loop_event = loop_event_create(fd,ptr,read_cb);
  if(loop_event == NULL){
    return E_FAIL;
  }

  struct epoll_event event;
  event.data.ptr=loop_event;
  event.events=event_flags;

  if(epoll_ctl(loop->epoll_fd,EPOLL_CTL_ADD,fd, &event) ==-1){
    logger(LOG_ERROR, "xps_loop_attach()", "epoll_ctl() failed");
    loop_event_destroy(loop_event);
    return E_FAIL;
  }

  vec_push(&loop->events, loop_event);
  return OK;

}

/**
 * Remove FD from epoll
 *
 * Find the instance of loop_event_t from loop->events that matches fd param
 * and detach FD from epoll. Destroy the loop_event_t instance and set the pointer
 * to NULL in loop->events list. Increment loop->n_null_events.
 *
 * @param loop : loop instnace from which to detach fd
 * @param fd : FD to be detached
 * @return : OK on success and E_FAIL on error
 */
int xps_loop_detach(xps_loop_t *loop, u_int fd) {
  assert(loop != NULL);

    for(int i=0; i<loop->events.length ; i++){
        loop_event_t * ev = loop->events.data[i];
        if(ev != NULL && ev->fd==fd){ 
            if(epoll_ctl(loop->epoll_fd,EPOLL_CTL_DEL,fd,NULL)==-1){
                logger(LOG_ERROR, "xps_loop_detach()", "epoll_ctl() failed");
                return E_FAIL;
            }
            loop_event_destroy(ev);
            loop->events.data[i]=NULL;
            loop->n_null_events++;
            return OK;
        }
    }
    logger(LOG_ERROR, "xps_loop_detach()", "FD not found in events vector");
    return E_FAIL;
}



void xps_loop_run(xps_loop_t *loop) {
  /* Validate params */
    while (1) {
        logger(LOG_DEBUG, "xps_loop_run()", "epoll wait");
        int n_events = epoll_wait(loop->epoll_fd, loop->epoll_events, MAX_EPOLL_EVENTS, -1);
        logger(LOG_DEBUG, "xps_loop_run()", "epoll wait over");

        logger(LOG_DEBUG, "xps_loop_run()", "handling %d events", n_events);

        // Handle events
        for (int i = 0; i < n_events; i++) {
            logger(LOG_DEBUG, "xps_loop_run()", "handling event");

            struct epoll_event curr_epoll_event = loop->epoll_events[i];
            loop_event_t *curr_event = curr_epoll_event.data.ptr;

            // Check if event still exists. Could have been destroyed due to prev event
            if (!valid_event(loop, curr_event)) {
                logger(LOG_DEBUG, "handle_epoll_events()", "event might have been destroyed.");
                continue;
            }

            // Close event
            if (curr_epoll_event.events & (EPOLLHUP | EPOLLERR)) {
                logger(LOG_DEBUG, "handle_epoll_events()", "EVENT / close");
                if(curr_event->close_cb != NULL) {
                    // Pass the ptr from loop_event_t as a parameter to the callback
                    curr_event->close_cb(curr_event->ptr);
                } else {
                    logger(LOG_WARNING, "handle_epoll_events()", "close_cb is NULL");
                }
            }

            if(!valid_event(loop, curr_event)) {
                logger(LOG_DEBUG, "handle_epoll_events()", "no event after closing");
                continue;
            }

            // Read event
            if (curr_epoll_event.events & EPOLLIN) {
                logger(LOG_DEBUG, "handle_epoll_events()", "EVENT / read");
                if (curr_event->read_cb != NULL) {
                    // Pass the ptr from loop_event_t as a parameter to the callback
                    curr_event->read_cb(curr_event->ptr);
                } else {
                    logger(LOG_WARNING, "handle_epoll_events()", "read_cb is NULL");
                }
            }

            if(!valid_event(loop, curr_event)) {
                logger(LOG_DEBUG, "handle_epoll_events()", "no event after reading");
                continue;
            }

            // Write event
            if (curr_epoll_event.events & EPOLLOUT) {
                logger(LOG_DEBUG, "handle_epoll_events()", "EVENT / write");
                if (curr_event->write_cb != NULL) {
                    // Pass the ptr from loop_event_t as a parameter to the callback
                    curr_event->write_cb(curr_event->ptr);
                } else {
                    logger(LOG_WARNING, "handle_epoll_events()", "write_cb is NULL");
                }
            }
        }
    }
}