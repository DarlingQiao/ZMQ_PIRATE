#include "czmq.h"
#include "mdp.h"



 
#ifdef _DEBUG  
#define Printf(format,...) printf("File: "__FILE__", Line: %05d: "format"\n", __LINE__, ##__VA_ARGS__)  
#else  
#define Printf(format,...)  
#endif


#define HEARTBEAT_LIVENESS  3       //  3-5 is 正常
#define HEARTBEAT_INTERVAL  2500    //  毫秒
#define HEARTBEAT_EXPIRY    HEARTBEAT_INTERVAL * HEARTBEAT_LIVENESS


#define MDP_DEFINE	0
#define ERR_SUCC_OK								0
#define ERR_CREATE_CONTEXT_FAIL		( -51 + MDP_DEFINE )			//创建上下文失败
#define ERR_INVALID_BROKER_OBJECT	( -52 + MDP_DEFINE )			//无效的代理对象
#define ERR_MSG_SIZE_EMPTY				( -53 + MDP_DEFINE )			//msg里数据少于一帧，即msg为空
#define ERR_IDENTITY_EMPTY				( -54 + MDP_DEFINE )			//身份信息为空
#define ERR_PUSHSTR_MSG_FAIL			( -55 + MDP_DEFINE )			//推入到msg的首帧位置失败
#define ERR_SEND_TO_WORKER_FAIL		( -56 + MDP_DEFINE )			//发送到服务器失败
#define ERR_INVALID_WORKER_OBJECT ( -57 + MDP_DEFINE )			//无效的服务器对象
#define ERR_APPEND_REQ_FAIL				( -58 + MDP_DEFINE )			//添加到请求队列尾失败
#define ERR_DATA_ILLEGAL					( -59 + MDP_DEFINE )			//数据不合法（数据包小于2[服务名+数据],）
#define ERR_APPEND_WAIT_FAIL			( -60 + MDP_DEFINE )			//添加到等待服务列表尾失败
#define ERR_INSERT_WORKER_FAIL		( -61 + MDP_DEFINE )			//添加到哈希表的工人失败
#define ERR_INSERT_SERVER_FAIL		( -62 + MDP_DEFINE )			//添加到哈希表的工人失败
#define ERR_INVALID_SERVER_OBJECT ( -63 + MDP_DEFINE )

//代理结构体
typedef struct {
	void *ctx;                  //  上下文
	void *socket;               //  客户端和服务端套接字
	int verbose;                //  打印
	char *endpoint;             //  代理绑定端口
	zhash_t *services;          //  代理服务链表
	zhash_t *workers;           //  服务链表
	zlist_t *waiting;           //  等待服务链表
	uint64_t heartbeat_at;      //  信号检测时间
} broker_t;

static broker_t * s_broker_new(int verbose ,int *err);
static int s_broker_destroy(broker_t **self_p);
static int s_broker_bind(broker_t *self, char *endpoint);
static int s_broker_worker_msg(broker_t *self, zframe_t *sender, zmsg_t *msg);
static int s_broker_client_msg(broker_t *self, zframe_t *sender, zmsg_t *msg);
static int s_broker_purge(broker_t *self);

//代理服务结构体
typedef struct {
	broker_t *broker;           //  代理实例
	char *name;                 //  服务名
	zlist_t *requests;          //  客户端请求列表
	zlist_t *waiting;           //  等待服务器列表
	size_t workers;             //  服务数量
} service_t;

static service_t *s_service_require(broker_t *self, zframe_t *service_frame, int *err);
static void s_service_destroy(void *argument);
static int s_service_dispatch(service_t *service, zmsg_t *msg);

//  服务结构体

typedef struct {
	broker_t *broker;           //  代理实例
	char *id_string;            //  服务身份ID
	zframe_t *identity;         //  路由的身份帧
	service_t *service;         //  存在服务
	int64_t expiry;             //  信号检测超时时间
} worker_t;

static worker_t *s_worker_require(broker_t *self, zframe_t *identity,int *err);
static int s_worker_delete(worker_t *self, int disconnect);
static void s_worker_destroy(void *argument);
static int s_worker_send(worker_t *self, char *command, char *option,zmsg_t *msg);
static int s_worker_waiting(worker_t *self);