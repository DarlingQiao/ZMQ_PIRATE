
#include "mdcliapi.h"
#include "zmq.h"

struct _mdcli_t {
	void * ctx;
	char *broker;
	void *client;              
	int verbose;               
	int timeout;                
	int retries;               
};

struct _mdcli_asyn_t {
  void *ctx_asyn;
  char *broker;
  void *client;
  int timeout;
  int verbose;
};

void s_mdcli_connect_to_broker(mdcli_t *self)
{
	if (self->client)
		zmq_close(self->client);
	

	self->client = zmq_socket(self->ctx, ZMQ_REQ);
	zmq_connect(self->client,self->broker);
	if (self->verbose)
		zclock_log("I: connecting to broker at %s...", self->broker);
}

mdcli_t *mdcli_new(char *broker, int verbose)
{
	assert(broker);

	mdcli_t *self = (mdcli_t *)zmalloc(sizeof(mdcli_t));

	self->ctx = zmq_ctx_new();
	self->broker = strdup(broker);
	self->verbose = verbose;
	self->timeout = 2500;           
	self->retries = 3;            

	s_mdcli_connect_to_broker(self);
	return self;
}


void mdcli_destroy(mdcli_t **self_p)
{
	assert(self_p);
	if (*self_p) {
		mdcli_t *self = *self_p;
		zmq_ctx_destroy(self->ctx);
		free(self->broker);
		free(self);
		*self_p = NULL;
	}
}


void mdcli_set_timeout(mdcli_t *self, int timeout)
{
	assert(self);
	self->timeout = timeout;
}


void mdcli_set_retries(mdcli_t *self, int retries)
{
	assert(self);
	self->retries = retries;
}


zmsg_t *mdcli_send(mdcli_t *self, char *service, zmsg_t **request_p)
{
	assert(self);
	assert(request_p);
	zmsg_t *request = *request_p;

	zmsg_pushstr(request, service);
	zmsg_pushstr(request, MDPC_CLIENT);
	if (self->verbose) {
		zclock_log("I: send request to '%s' service:", service);
		zmsg_dump(request);
	}
	int retries_left = self->retries;
	while (retries_left && !zctx_interrupted) {
		zmsg_t *msg = zmsg_dup(request);
		zmsg_send(&msg, self->client);

		zmq_pollitem_t items[] = {
				{ self->client, 0, ZMQ_POLLIN, 0 }
		};
		int rc = zmq_poll(items, 1, self->timeout * ZMQ_POLL_MSEC);
		if (rc == -1)
			break;         

		if (items[0].revents & ZMQ_POLLIN) {
			zmsg_t *msg = zmsg_recv(self->client);
			if (self->verbose) {
				zclock_log("I: received reply:");
				zmsg_dump(msg);
			}
			
			assert(zmsg_size(msg) >= 3);

			zframe_t *header = zmsg_pop(msg);
			assert(zframe_streq(header, MDPC_CLIENT));
			zframe_destroy(&header);

			zframe_t *reply_service = zmsg_pop(msg);
			assert(zframe_streq(reply_service, service));
	
			zframe_destroy(&reply_service);

			zmsg_destroy(&request);
			return msg;    
		}
		else if (--retries_left) {
			if (self->verbose)
				zclock_log("W: no reply, reconnecting...");
			s_mdcli_connect_to_broker(self);
			}
			else {
				if (self->verbose)
					zclock_log("W: permanent error, abandoning");
				break;          
			}
	}
	if (zctx_interrupted)
		printf("W: interrupt received, killing client...\n");
	zmsg_destroy(&request);
	return NULL;
}


//=========异步接收发送===========//
mdcli_asyn_t *mdcli_asyn_new(char *proxy_addr, int verbose)
{
  assert(proxy_addr);

  mdcli_asyn_t *self = (mdcli_asyn_t *)zmalloc(sizeof(mdcli_asyn_t));

  self->ctx_asyn = zmq_ctx_new();
  self->broker = strdup(proxy_addr);
  self->verbose = verbose;
  self->timeout = 2500;
  
  self->client = zmq_socket(self->ctx_asyn, ZMQ_DEALER);
  zmq_connect(self->client, self->broker);
  if (self->verbose)
    zclock_log("I: connecting to broker at %s...", self->broker);

  return self;
}

void mdcli_asyn_destroy(mdcli_asyn_t **self_p)
{
  assert(self_p);
  if (*self_p) {
    mdcli_asyn_t *self = *self_p;
    zmq_ctx_destroy(self->ctx_asyn);
    free(self->broker);
    free(self);
    *self_p = NULL;
  }
}

int mdcli_asyn_send(mdcli_asyn_t *self, char *service, zmsg_t **request_p)
{
  assert(self);
  assert(request_p);
  zmsg_t *request = *request_p;

  zmsg_pushstr(request, service);
  zmsg_pushstr(request, MDPC_CLIENT);
  zmsg_pushstr(request, "");
  if (self->verbose) {
    zclock_log("I: send request to '%s' service:", service);
    zmsg_dump(request);
  }
  int rec = zmsg_send(&request, self->client);
  if (rec != 0) {
    return -1;
  }
  return 0;
}

zmsg_t *mdcli_asyn_recv(mdcli_asyn_t *self)
{
  assert(self);
  zmq_pollitem_t items[] = {
      { self->client, 0, ZMQ_POLLIN, 0 }
  };
  int rc = zmq_poll(items, 1, self->timeout * ZMQ_POLL_MSEC);
  if (rc == -1)
    return NULL;

  if (items[0].revents & ZMQ_POLLIN) {
    zmsg_t *msg = zmsg_recv(self->client);
    if (self->verbose) {
      zclock_log("I: received reply:");
      zmsg_dump(msg);
    }
    assert(zmsg_size(msg) >= 4);

    zframe_t *empty = zmsg_pop(msg);
    assert(zframe_streq(empty, ""));
    zframe_destroy(&empty);

    zframe_t *header = zmsg_pop(msg);
    assert(zframe_streq(header, MDPC_CLIENT));
    zframe_destroy(&header);

    zframe_t *service = zmsg_pop(msg);
    zframe_destroy(&service);

    return msg;
  }
  return NULL;
}