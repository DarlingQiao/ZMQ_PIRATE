// zmq_mdp_proxy.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "mdpBroker.h"

//代理构造
/*******修改创建套接字方法 **********/

static broker_t *s_broker_new(int verbose, int *err)
{
  *err = ERR_SUCC_OK;
  broker_t *self = (broker_t *)zmalloc(sizeof(broker_t));

  //  初始化代理属性
  self->ctx = zmq_ctx_new();
  assert(self->ctx);
  if (self->ctx == NULL) {
    *err = ERR_CREATE_CONTEXT_FAIL;
    return NULL;
  }
  self->socket = zmq_socket(self->ctx, ZMQ_ROUTER);
  assert(self->ctx);
  if (self->socket == NULL) {
    *err = zmq_errno() + MDP_DEFINE;
    return NULL;
  }
  self->verbose = verbose;
  self->services = zhash_new();
  self->workers = zhash_new();
  self->waiting = zlist_new();
  self->heartbeat_at = zclock_time() + HEARTBEAT_INTERVAL;
  return self;
}
//代理析构
static int s_broker_destroy(broker_t **self_p)
{
  int err = ERR_SUCC_OK;
  assert(self_p);
  if (self_p == NULL)	{
    err = ERR_INVALID_BROKER_OBJECT;
    return err;
  }
  if (*self_p) {
    broker_t *self = *self_p;
    err = zmq_ctx_term(self->ctx);
    assert(err == 0);
    if (err != 0)	{
      err = zmq_errno() + MDP_DEFINE;
      return err;
    }
    zhash_destroy(&self->services);
    zhash_destroy(&self->workers);
    zlist_destroy(&self->waiting);
    free(self);
    *self_p = NULL;
  }
  return err;
}

//此方法将代理实例绑定到端点 可以多次调用 请注意 MDP为客户端和服务器使用单个套接字：

//代理绑定

int s_broker_bind(broker_t *self, char *endpoint)
{
  //zsocket_bind(self->socket, endpoint);
  int err = zmq_bind(self->socket, endpoint);
  assert(err == 0);
  if (err != 0)	{
    err = zmq_errno() + MDP_DEFINE;
    return err;
  }
  zclock_log("I: MDP broker/0.2.0 is active at %s", endpoint);
	return 0;
}

//此方法处理由服务发送到代理的一个READY，REPLY，HEARTBEAT或DISCONNECT消息：
//（就绪，应答，信号检测，断开连接）
static int s_broker_worker_msg(broker_t *self, zframe_t *sender, zmsg_t *msg)
{
	int err = ERR_SUCC_OK;
	assert(zmsg_size(msg) >= 1);     //  至少一个命令
	if (zmsg_size(msg) < 1)	{
		err = ERR_MSG_SIZE_EMPTY;
		return err;
	}
	zframe_t *command = zmsg_pop(msg);
	char *id_string = zframe_strhex(sender);
	int worker_ready = (zhash_lookup(self->workers, id_string) != NULL);
	free(id_string);

  worker_t *worker = s_worker_require(self, sender, &err);
  if (worker == NULL)	{
    Printf("s_worker_require err = %d", err);
    return err;
  }

  if (zframe_streq(command, MDPW_READY)) {              //注册
    Printf("注册\n");
    if (worker_ready) {                                  //  不是会话中的第一个命令
      err = s_worker_delete(worker, 1);
      if (err != 0) {
        Printf("删除失败 %d", err);
        return err;
      }
      Printf("不是会话中的第一个命令===删除当前服务\n");
    }
    else if (zframe_size(sender) >= 4 && memcmp(zframe_data(sender), "mmi.", 4) == 0) {//  保留服务名
      err = s_worker_delete(worker, 1);
      if (err != 0) {
        Printf("删除失败 %d", err);
        return err;
      }
      Printf("保留服务名===删除当前服务\n");
    }
    else {
      //  将服务加到队列中，标示空闲
      Printf("服务添加到队列，标示空闲\n");
      zframe_t *service_frame = zmsg_pop(msg);
      worker->service = s_service_require(self, service_frame, &err);
      if (worker->service == NULL)	{
        Printf("s_service_require err = %d", err);
        return err;
      }
      worker->service->workers++;
      err = s_worker_waiting(worker);
      if (err != 0) {
        Printf("s_worker_waiting err = %d", err);
        return err;
      }
      zframe_destroy(&service_frame);
    }
  }
  else if (zframe_streq(command, MDPW_REPLY)) {         //应答
    Printf("应答\n");
    if (worker_ready) {
      Printf("删除并保存客户端的封包，把协议头和服务名插进去，重新封包\n");
      //  删除并保存客户端的封包
      // 把协议头和服务名插进去，重新封包
      zframe_t *client = zmsg_unwrap(msg);
      zmsg_pushstr(msg, worker->service->name);
      zmsg_pushstr(msg, MDPC_CLIENT);
      zmsg_wrap(msg, client);
      zmsg_send(&msg, self->socket);
      err = s_worker_waiting(worker);
    }
    else {
      Printf("删除当前服务\n");
      err = s_worker_delete(worker, 1);
    }
  }
  else if (zframe_streq(command, MDPW_HEARTBEAT)) {     //心跳包
    Printf("心跳包\n");
    if (worker_ready)
      worker->expiry = zclock_time() + HEARTBEAT_EXPIRY;
    else {
      Printf("删除当前服务\n");
      err = s_worker_delete(worker, 1);
    }
  }
  else if (zframe_streq(command, MDPW_DISCONNECT)) {     //断线包
    Printf("断线包\n");
    err = s_worker_delete(worker, 0);
    Printf("删除当前服务\n");
  }
  else {
    Printf("无效包\n");
    zclock_log("E: 无效输入消息");                      //无效包
    zmsg_dump(msg);
  }
  free(command);
  zmsg_destroy(&msg);
  return err;
}

// 处理客户端的请求

static int s_broker_client_msg(broker_t *self, zframe_t *sender, zmsg_t *msg)
{
  int err = ERR_SUCC_OK;
  assert(zmsg_size(msg) >= 2);     //  服务名+数据

  zframe_t *service_frame = zmsg_pop(msg);
  service_t *service = s_service_require(self, service_frame, &err);
  if (service == NULL)	{
    Printf("服务链表为NULL\n");
    return err;
  }


  //  将回复身份设置回客户端发件人
  zmsg_wrap(msg, zframe_dup(sender));

  //  如果得到MMI服务请求，就在内服处理它
  if (zframe_size(service_frame) >= 4
    && memcmp(zframe_data(service_frame), "mmi.", 4) == 0) {
    char *return_code;
    if (zframe_streq(service_frame, "mmi.server")) {
      char *name = zframe_strdup(zmsg_last(msg));
      service_t *service = (service_t *)zhash_lookup(self->services, name);
      return_code = service && service->workers ? "200" : "404";
      free(name);
    }
    else
      return_code = "501";

    zframe_reset(zmsg_last(msg), return_code, strlen(return_code));

    //  删除并保存客户端的封包
    // 把协议头和服务名插进去，重新封包
    zframe_t *client = zmsg_unwrap(msg);
    zmsg_prepend(msg, &service_frame);
    zmsg_pushstr(msg, MDPC_CLIENT);
    zmsg_wrap(msg, client);
    zmsg_send(&msg, self->socket);

  }
  else
    err = s_service_dispatch(service, msg); //  将消息分配给请求的服务
  if (err != 0)	{
    Printf("%d", err);
  }
  zframe_destroy(&service_frame);
  return err;
}

//这个方法会删除一个没有ping我们的空闲服务

static int s_broker_purge(broker_t *self)
{
  int err = ERR_SUCC_OK;
  worker_t *worker = (worker_t *)zlist_first(self->waiting);
  if (worker == NULL) {
    Printf("worker 队列为空\n");
    return err;
  }
  while (worker) {
    if (zclock_time() < worker->expiry) {
      Printf("服务在线\n");
      break;                                  //  服务在线 退出
    }
    if (self->verbose)
      zclock_log("I: 删除过期的服务: %s", worker->id_string);

    Printf("删除过期的服务:%s\n", worker->id_string);
    err = s_worker_delete(worker, 0);
    if (err != 0)	{
      return err;
    }
    worker = (worker_t *)zlist_first(self->waiting);
  }
  return err;
}

// 通过名字查找服务，如果服务不存在，创建一个新的服务

static service_t *s_service_require(broker_t *self, zframe_t *service_frame, int *err)
{
  *err = ERR_SUCC_OK;
  assert(service_frame);
  char *name = zframe_strdup(service_frame);

  service_t *service = (service_t *)zhash_lookup(self->services, name);
  if (service == NULL) {
    Printf("zhash_lookup 没有找到%s的服务，所以新建一条服务\n", name);
    service = (service_t *)zmalloc(sizeof(service_t));
    service->broker = self;
    service->name = name;
    service->requests = zlist_new();
    service->waiting = zlist_new();

    int rec = zhash_insert(self->services, name, service);
    if (rec == -1) {
      Printf("key已经存在，返回-1并保留现有项目不变\n");
    }
    zhash_freefn(self->services, name, s_service_destroy);
    if (self->verbose)
      zclock_log("I: 添加服务: %s", name);
  }
  else {
    Printf("zhash_lookup 找到%s的服务,删除该条\n", name);
    free(name);
  }
  return service;
}

//  但服务从 broker->services 中删除，都自动调用服务的析构函数

static void s_service_destroy(void *argument)
{
  service_t *service = (service_t *)argument;
  while (zlist_size(service->requests)) {
    zmsg_t *msg = (zmsg_t *)zlist_pop(service->requests);
    zmsg_destroy(&msg);
  }
  zlist_destroy(&service->requests);
  zlist_destroy(&service->waiting);
  free(service->name);
  free(service);
}

//  请求发送给等候的服务

static int s_service_dispatch(service_t *self, zmsg_t *msg)
{
  int err = ERR_SUCC_OK;
  assert(self);
  if (self == NULL)	{
    err = ERR_INVALID_SERVER_OBJECT;
    return err;
  }
  if (msg) {
    if (zlist_append(self->requests,msg) == -1) {
      Printf("添加消息失败\n");
      return err = ERR_APPEND_WAIT_FAIL;
    }
  }
  //if (zlist_size(self->waiting) == 0 || zlist_size(self->requests) == 0)
  //  return ERR_SUCC_OK;
  //else if (msg) {                   //  消息排队
  //  if (zlist_append(self->requests, msg) == -1) {
  //    Printf("添加消息失败\n");
  //    err = ERR_APPEND_WAIT_FAIL;
  //    return err;
  //  }
  //}
  err = s_broker_purge(self->broker);
  if (err != 0)	{
    return err;
  }

  while (zlist_size(self->waiting) && zlist_size(self->requests)) {
    worker_t *worker = (worker_t *)zlist_pop(self->waiting);
    zlist_remove(self->broker->waiting, worker);
    zmsg_t *msg = (zmsg_t *)zlist_pop(self->requests);
    s_worker_send(worker, MDPW_REQUEST, NULL, msg);
    zmsg_destroy(&msg); 
  }
  return err;
}

// 通过身份查找服务，如果没有创建一个新的

static worker_t *s_worker_require(broker_t *self, zframe_t *identity, int *err)
{
  *err = ERR_SUCC_OK;
  assert(identity);
  if (identity == NULL)	{
    *err = ERR_IDENTITY_EMPTY;
    return NULL;
  }

  //  self->workers保存服务身份信息
  char *id_string = zframe_strhex(identity);
  worker_t *worker = (worker_t *)zhash_lookup(self->workers, id_string);

  if (worker == NULL) {
    worker = (worker_t *)zmalloc(sizeof(worker_t));
    worker->broker = self;
    worker->id_string = id_string;
    worker->identity = zframe_dup(identity);
    zhash_insert(self->workers, id_string, worker);
    zhash_freefn(self->workers, id_string, s_worker_destroy);
    if (self->verbose)
      zclock_log("I: 注册新服务: %s", id_string);
  }
	else {
		Printf("%s", id_string);
		free(id_string);
	}
    
  return worker;
}

// 删除当前服务

static int s_worker_delete(worker_t *self, int disconnect)
{
  int err = ERR_SUCC_OK;
  assert(self);
  if (self == NULL)	{
    err = ERR_INVALID_WORKER_OBJECT;
    return err;
  }
  if (disconnect)
    err = s_worker_send(self, MDPW_DISCONNECT, NULL, NULL);
  if (err != 0)	{
    return err;
  }
  if (self->service) {
    zlist_remove(self->service->waiting, self);
    self->service->workers--;
  }
  zlist_remove(self->broker->waiting, self);

  zhash_delete(self->broker->workers, self->id_string);
  return err;
}

//  当服务从 broker->workers.中删除，都调用工人析构函数

static void s_worker_destroy(void *argument)
{
  worker_t *self = (worker_t *)argument;
  zframe_destroy(&self->identity);
  free(self->id_string);
  free(self);
}

//此方法格式化一个命令并发送给服务。
//可以提供一个命令选项和一个消息负载：

static int s_worker_send(worker_t *self, char *command, char *option, zmsg_t *msg)
{
  int err = ERR_SUCC_OK;
  msg = msg ? zmsg_dup(msg) : zmsg_new();

  //  在消息开头叠加消息包
  if (option)
    err = zmsg_pushstr(msg, option);


  if (err == 0 && zmsg_pushstr(msg, command) == 0 && zmsg_pushstr(msg, MDPW_WORKER) == 0)	{
    err = ERR_SUCC_OK;
  }
  else {
    err = ERR_PUSHSTR_MSG_FAIL;
    return err;
  }
  // 	err = zmsg_pushstr(msg, command);
  //   err = zmsg_pushstr(msg, MDPW_WORKER);

  //  在消息开头叠加路由包
  zmsg_wrap(msg, zframe_dup(self->identity));

  if (self->broker->verbose) {
    zclock_log("I: 发送服务中 %s ", mdps_commands[(int)*command]);
    zmsg_dump(msg);
  }
  if (zmsg_send(&msg, self->broker->socket) == -1) {
    Printf("发送服务命令失败：%s\n", mdps_commands[(int)*command]);
    err = ERR_SEND_TO_WORKER_FAIL;
    return err;
  }
  Printf("发送服务命令成功：%s\n", mdps_commands[(int)*command]);
  return err;
}

//  等待中的服务

static int s_worker_waiting(worker_t *self)
{
  //  排队到代理和服务列表
  assert(self->broker);
  zlist_append(self->broker->waiting, self);
  zlist_append(self->service->waiting, self);
  self->expiry = zclock_time() + HEARTBEAT_EXPIRY;
  return s_service_dispatch(self->service, NULL);
}

//处理代理上套接字的消息

zmq_thread_fn(sub_pub_proxy);

void sub_pub_proxy(void *arg)
{
	char * port_pull = strtok((char*)arg, ";");
	char * port_pub = strtok(NULL, ";");
  void *context = zmq_ctx_new();
  void *receiver = zmq_socket(context, ZMQ_PULL);
  //zmq_bind(receiver, "tcp://*:5558");
	int rc = zmq_bind(receiver, port_pull);
	Printf("pull bind: %s",port_pull);

  void *controller = zmq_socket(context, ZMQ_PUB);
  //zmq_bind(controller, "tcp://*:5559");
	rc = zmq_bind(controller, port_pub);
	Printf("pub bind: %s", port_pub);
	Printf("线程运行中。。。");
  while (true) {
    zmsg_t *msg = zmsg_recv(receiver);
		if (msg == NULL) {
			continue;
		}
    rc = zmsg_send(&msg, controller);
		if (rc != 0)	{
			continue;
		}
    zmsg_destroy(&msg);
  }
  rc = zmq_close(receiver);
  rc = zmq_close(controller);
  rc = zmq_ctx_destroy(context);
}

int main(int argc, char *argv[])
{
  int verbose = (argc > 1 && streq(argv[1], "-v"));
  int err = 0;

	//读取配置文件
	zconfig_t *root = zconfig_load("config.cfg");
	if (root == NULL)	{
		Printf("没找到配置文件,配置应该放在exe程序的同一级目录下，配置文件名为 config.cfg");
		zmq_sleep(3);
		return -1;
	}
	char* filename = (char*)zconfig_filename(root);
	//"tcp://*:5555"
	char *proxy_port = zconfig_get(root, "/proxy/route", "tcp://*:5555");
	
	char proxy_config[64] = { 0 };
	strcpy(proxy_config, proxy_port);
	char config_sink[256] = { 0 };
	char* pull_port = zconfig_get(root, "/proxy/pull", "tcp://*:5558");
	char* pub_port = zconfig_get(root, "/proxy/publish", "tcp://*:5559");

	strcat(config_sink, pull_port);
	strcat(config_sink, ";");
	strcat(config_sink, pub_port);
	strcat(config_sink, ";");

	zconfig_destroy(&root);

	//起接收器线程
	zmq_threadstart(sub_pub_proxy, config_sink);

	
  broker_t *self = s_broker_new(verbose, &err);
  assert(self);
	err = s_broker_bind(self, proxy_config);
  if (err != 0)	{
		Printf("绑定失败%s, %d, %s", proxy_config, err, zmq_strerror(zmq_errno()));
		zmq_sleep(3);
    return 0;
  }
	Printf("绑定成功 = %s", proxy_config);

  //  获取和处理消息
  while (true) {
    err = 0;
    zmq_pollitem_t items[] = {
        { self->socket, 0, ZMQ_POLLIN, 0 } };
    int rc = zmq_poll(items, 1, HEARTBEAT_INTERVAL * ZMQ_POLL_MSEC);
    if (rc == -1)
      break;

    // 处理下个消息
    if (items[0].revents & ZMQ_POLLIN) {
      zmsg_t *msg = zmsg_recv(self->socket);
      if (!msg)
        break;
      if (self->verbose) {
        zclock_log("I: 收到消息:");
        zmsg_dump(msg);
      }
      zframe_t *sender = zmsg_pop(msg);
      char *str = zframe_strdup(sender);
      zframe_t *empty = zmsg_pop(msg);
      str = zframe_strdup(empty);
      zframe_t *header = zmsg_pop(msg);
      str = zframe_strdup(header);

      if (zframe_streq(header, MDPC_CLIENT)) {
        err = 0;
        err = s_broker_client_msg(self, sender, msg);
        if (err != 0)	{
          Printf("客户端消息处理失败 %d", err);
        }
      }
      else if (zframe_streq(header, MDPW_WORKER)) {
        err = 0;
        err = s_broker_worker_msg(self, sender, msg);
        if (err != 0)	{
          Printf("服务器消息处理失败 %d", err);
        }
      }
      else {
        zclock_log("E: 无效消息:");
        zmsg_dump(msg);
        zmsg_destroy(&msg);
      }
      zframe_destroy(&sender);
      zframe_destroy(&empty);
      zframe_destroy(&header);
    }
    //断开并删除任何过期的服务
    //如果需要，可以向空闲的服务发送心跳
    if (zclock_time() > self->heartbeat_at) {
      s_broker_purge(self);
      worker_t *worker = (worker_t *)zlist_first(self->waiting);
      while (worker) {
        err = s_worker_send(worker, MDPW_HEARTBEAT, NULL, NULL);
        if (err != 0)	{
          Printf("心跳失败 %d", err);
        }
        worker = (worker_t *)zlist_next(self->waiting);
      }
      self->heartbeat_at = zclock_time() + HEARTBEAT_INTERVAL;
    }
  }
  if (zctx_interrupted)
    Printf("W：中断接收，关机...\n");

  err = s_broker_destroy(&self);
  if (err != 0)	{
    Printf("代理失败 %d", err);
  }
  return 0;
}


