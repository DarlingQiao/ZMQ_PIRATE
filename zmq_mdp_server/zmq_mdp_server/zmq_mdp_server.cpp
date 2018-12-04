// zmq_mdp_server.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "zmq_mdp_server.h"
#include <stdio.h>
#include <assert.h>

int send_proxy(mdwrk_t *self, char *command, char *option, zmsg_t *msg) {

  msg = msg ? zmsg_dup(msg) : zmsg_new();
  if (option) {
    if (zmsg_pushstr(msg, option) != 0) {
      return -1;
    }
  }

  if (zmsg_pushstr(msg, command) != 0 || zmsg_pushstr(msg, MDPW_WORKER) != 0
    || zmsg_pushstr(msg, "") != 0) {
    return -1;
  }

  int rec = zmsg_send(&msg, self->worker);
  if (rec == -1) {
    return -1;
  }
  return 0;
}

int connect_proxy(mdwrk_t *self) {

  if (self->worker) {
    if (zmq_close(self->worker) != 0) {
      return -1;
    }
  }

  self->worker = zmq_socket(self->ctx, ZMQ_DEALER);
  int ret = zmq_connect(self->worker, self->broker);
  if (ret != 0) {
    return zmq_errno();
  }
  ret = send_proxy(self, MDPW_READY, self->service, NULL);
  if (ret == -1) {
    return -1;
  }

  self->liveness = HEARTBEAT_LIVENESS;
  self->heartbeat_at = zclock_time() + self->heartbeat;
  return 0;
}

mdwrk_t  *server_new(char *proxy_addr, char *service_name)//(char *proxy_addr, char *service_name, mdwrk_t *self) {
{
  if (proxy_addr == NULL || service_name == NULL) {
    return 0;
  }

  mdwrk_t *self = (mdwrk_t *)zmalloc(sizeof(mdwrk_t));
  self->ctx = zmq_ctx_new();
  self->broker = strdup(proxy_addr);
  self->service = strdup(service_name);
  self->heartbeat = 2500;
  self->reconnect = 2500;

  int ret = connect_proxy(self);
  if (ret == -1) {
    return NULL;
  }
 
  return self;
}

void  server_destroy(mdwrk_t **self_p) {

  if (*self_p == NULL) {
    return;
  }
  if (*self_p) {
    mdwrk_t *self = *self_p;
    //int ret = zmq_ctx_destroy(self->ctx);
    int ret = zmq_close(self->worker);
    if (ret == -1) {
      return;
    }
    ret = zmq_ctx_term(self->ctx);
    if (ret == -1) {
      return;
    }

    free(self->broker);
    free(self->service);
    free(self);
    *self_p = NULL;
  }
}

void  server_setliveness(mdwrk_t *self, int liveness) {
  self->heartbeat = liveness;
}

void  server_setheartbeat(mdwrk_t *self, int heartbeat) {
  self->heartbeat = heartbeat;
}

void  server_setreconnect(mdwrk_t *self, int reconnect) {
  self->reconnect = reconnect;
}

zmsg_t*  server_recv(mdwrk_t *self) {
  
  zmsg_t *reply = NULL;
  server_reply(self, &reply);
  self->expect_reply = 1;

  while (true) {
    zmq_pollitem_t items[] = {
        { self->worker, 0, ZMQ_POLLIN, 0 } };
    int rc = zmq_poll(items, 1, self->heartbeat * ZMQ_POLL_MSEC);
    if (rc == -1)
      break;

    if (items[0].revents & ZMQ_POLLIN) {
      zmsg_t *msg = zmsg_recv(self->worker);
      if (!msg)
        break;

      self->liveness = HEARTBEAT_LIVENESS;

      assert(zmsg_size(msg) >= 3);

      zframe_t *empty = zmsg_pop(msg);
      assert(zframe_streq(empty, ""));
      zframe_destroy(&empty);

      zframe_t *header = zmsg_pop(msg);
      assert(zframe_streq(header, MDPW_WORKER));
      zframe_destroy(&header);

      zframe_t *command = zmsg_pop(msg);

      if (zframe_streq(command, MDPW_REQUEST)) {    //请求包

        self->reply_to = zmsg_unwrap(msg);
        zframe_destroy(&command);
        return msg;
      }
      else if (zframe_streq(command, MDPW_HEARTBEAT))    //心跳包
        ;
      else if (zframe_streq(command, MDPW_DISCONNECT)) {   //断开连接
        int ret = connect_proxy(self);
        if (ret == -1) {
          ;
        }
      }
      else
        ;// printf("代理发送的是无效的输入消息\n");

      zframe_destroy(&command);
      zmsg_destroy(&msg);
    }
    else if (--self->liveness == 0) {
      zclock_sleep(self->reconnect);
      int ret = connect_proxy(self);
      if (ret == -1) {
        ;
      }
    }
    if (zclock_time() > self->heartbeat_at) {
      send_proxy(self, MDPW_HEARTBEAT, NULL, NULL);
      self->heartbeat_at = zclock_time() + self->heartbeat;
    }
  }
  return NULL;
}

void  server_reply(mdwrk_t *self, zmsg_t **reply_p) {

  zmsg_t *reply = *reply_p;
  if (reply) {
    zmsg_wrap(reply, self->reply_to);
    send_proxy(self, MDPW_REPLY, NULL, reply);
    zmsg_destroy(reply_p);
  }
}

void message_set(zmsg_t *request, const char *str) {

  zframe_reset(zmsg_last(request), str, strlen(str));
}


//==============发布订阅模式===============//

mdwrk_pub_t *server_pub_new(char *pull_addr)
{
  mdwrk_pub_t *self = (mdwrk_pub_t *)zmalloc(sizeof(mdwrk_pub_t));
  self->ctx_pub = zmq_ctx_new();
  self->publisher = zmq_socket(self->ctx_pub, ZMQ_PUSH);
  int rc = zmq_connect(self->publisher, pull_addr);
  assert(rc == 0);
  return self;
}

int server_pub_send(mdwrk_pub_t *self, char *category, char *message)
{
  if (category == NULL) {
    return -1;
  }
  zmsg_t *msg = zmsg_new();
  zmsg_pushstr(msg, message);
  zmsg_pushstr(msg, category);
  if (zmsg_send(&msg, self->publisher) == -1) {
    zmsg_destroy(&msg);
    return -1;
  }
  zmsg_destroy(&msg);
	return 0;
}

void server_pub_destroy(mdwrk_pub_t **self_p)
{
  if (*self_p == NULL) {
    return;
  }
  if (*self_p) {
    mdwrk_pub_t *self = *self_p;
   // int ret = zmq_ctx_destroy(self->ctx_pub);
    int ret = zmq_close(self->publisher);
    if (ret == -1) {
      return;
    }
    ret = zmq_ctx_term(self->ctx_pub);
    if (ret == -1) {
      return;
    }
    free(self);
    *self_p = NULL;
  }
}