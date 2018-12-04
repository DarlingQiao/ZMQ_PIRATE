#ifndef __MDCLIAPI_H__
#define __MDCLIAPI_H__

#include "czmq.h"
#include "mdp.h"

#ifdef __cplusplus
extern "C" {
#endif

  typedef struct _mdcli_t mdcli_t;
  typedef struct _mdcli_asyn_t mdcli_asyn_t;
	
  //  同步发送接收
	mdcli_t *mdcli_new(char *broker, int verbose);
	void mdcli_destroy(mdcli_t **self_p);
	void mdcli_set_timeout(mdcli_t *self, int timeout);
	void mdcli_set_retries(mdcli_t *self, int retries);
	zmsg_t *mdcli_send(mdcli_t *self, char *service, zmsg_t **request_p);

  //异步发送接收
  mdcli_asyn_t *mdcli_asyn_new(char *broker, int verbose);
  void mdcli_asyn_destroy(mdcli_asyn_t **self_p);
  int mdcli_asyn_send(mdcli_asyn_t *self, char *service, zmsg_t **request_p);
  zmsg_t *mdcli_asyn_recv(mdcli_asyn_t *self);

#ifdef __cplusplus
}
#endif

#endif
