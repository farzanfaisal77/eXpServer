#include "xps.h"

xps_core_t *core;

void sigint_handler(int signum);

int core_create(xps_config_t *config);

int main(int argc, char *argv[]) {
  	signal(SIGINT, sigint_handler);
	
	xps_cliargs_t *cliargs = xps_cliargs_create(argc, argv);

	if(!cliargs) {
        logger(LOG_ERROR, "main()", "xps_cliargs_create() failed");
        return -1;
    }
	xps_config_t *config = xps_config_create(cliargs->config_path);
	
	if (!config){
		logger(LOG_ERROR, "main()", "xps_config_create() failed");
        free(cliargs);
        return -1;
	}

	if (core_create(config) != OK){
		logger(LOG_ERROR, "main()", "core_create() failed");
        xps_config_destroy(config);
        free(cliargs);
        return -1;
	}

	xps_core_start(core);

}

int core_create(xps_config_t *config){
	assert(config);

	core = xps_core_create(config);

	for(int i=0; i<config->_all_listeners.length; i++){
		xps_config_listener_t *config_listener = config->_all_listeners.data[i];
		xps_listener_t *listener = xps_listener_create(config_listener->host, config_listener->port);
		if(listener == NULL) {
            logger(LOG_ERROR, "core_create()", "xps_listener_create() failed for %s:%u", config_listener->host, config_listener->port);
            xps_core_destroy(core);
            return E_FAIL;
        }
		listener->core = core;

		if(xps_loop_attach(core->loop, listener->sock_fd, EPOLLIN | EPOLLET, listener, listener_connection_handler, NULL, NULL) != OK) {
            logger(LOG_ERROR, "core_create()", "xps_loop_attach() failed for listener %s:%u", config_listener->host, config_listener->port);
            close(listener->sock_fd);
            free(listener);
            xps_core_destroy(core);
            return E_FAIL;
        }
		vec_push(&(core->listeners), listener);

        logger(LOG_INFO, "core_create()", "listener created for %s:%u", config_listener->host, config_listener->port);
	}

	return OK;
}

void sigint_handler(int signum) {
	logger(LOG_WARNING, "sigint_handler()", "SIGINT received");

	xps_core_destroy(core);

	exit(EXIT_SUCCESS);
}