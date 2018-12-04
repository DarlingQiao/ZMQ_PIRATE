
#ifndef __ZMQ__MDP_SERVER__H__
#define __ZMQ__MDP_SERVER__H__

#include "zmq.h"
#include "czmq.h"
#include "mdp.h"

#ifndef MDP_SERVER_API
#define MDP_SERVER_API __declspec(dllexport)
#else
#define MDP_SERVER_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define HEARTBEAT_LIVENESS  3       // 

  struct _mdwrk_t {

    void * ctx;
    char *broker;
    char *service;
    void *worker;
    uint64_t heartbeat_at;
    size_t liveness;
    int heartbeat;
    int reconnect;
    int expect_reply;
    zframe_t *reply_to;
  };
  typedef struct _mdwrk_t mdwrk_t;

  struct _mdwrk_pub_t {
    void *ctx_pub;
    void *publisher;
  };
  typedef struct _mdwrk_pub_t mdwrk_pub_t;
  

 // MDP_SERVER_API int server_new(char *proxy_addr, char *service_name);
  MDP_SERVER_API mdwrk_t *server_new(char *proxy_addr, char *service_name);

  MDP_SERVER_API void server_destroy(mdwrk_t **self_p);

  MDP_SERVER_API void server_setliveness(mdwrk_t *self, int liveness);

  MDP_SERVER_API void server_setheartbeat(mdwrk_t *self, int heartbeat);

  MDP_SERVER_API void server_setreconnect(mdwrk_t *self, int reconnect);

  MDP_SERVER_API zmsg_t *server_recv(mdwrk_t *self);

  MDP_SERVER_API void server_reply(mdwrk_t *self, zmsg_t **reply_p);

  MDP_SERVER_API void message_set(zmsg_t *request,const char *str);

  //===========发布订阅模式===========

  MDP_SERVER_API mdwrk_pub_t *server_pub_new(char *pull_addr);

  MDP_SERVER_API int server_pub_send(mdwrk_pub_t *self, char *category, char *message);

  MDP_SERVER_API void server_pub_destroy(mdwrk_pub_t **self_p);



#ifdef __cplusplus
}
#endif

#endif