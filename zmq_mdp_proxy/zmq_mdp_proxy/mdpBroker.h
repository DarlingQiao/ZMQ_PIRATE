#include "czmq.h"
#include "mdp.h"



 
#ifdef _DEBUG  
#define Printf(format,...) printf("File: "__FILE__", Line: %05d: "format"\n", __LINE__, ##__VA_ARGS__)  
#else  
#define Printf(format,...)  
#endif


#define HEARTBEAT_LIVENESS  3       //  3-5 is ����
#define HEARTBEAT_INTERVAL  2500    //  ����
#define HEARTBEAT_EXPIRY    HEARTBEAT_INTERVAL * HEARTBEAT_LIVENESS


#define MDP_DEFINE	0
#define ERR_SUCC_OK								0
#define ERR_CREATE_CONTEXT_FAIL		( -51 + MDP_DEFINE )			//����������ʧ��
#define ERR_INVALID_BROKER_OBJECT	( -52 + MDP_DEFINE )			//��Ч�Ĵ������
#define ERR_MSG_SIZE_EMPTY				( -53 + MDP_DEFINE )			//msg����������һ֡����msgΪ��
#define ERR_IDENTITY_EMPTY				( -54 + MDP_DEFINE )			//�����ϢΪ��
#define ERR_PUSHSTR_MSG_FAIL			( -55 + MDP_DEFINE )			//���뵽msg����֡λ��ʧ��
#define ERR_SEND_TO_WORKER_FAIL		( -56 + MDP_DEFINE )			//���͵�������ʧ��
#define ERR_INVALID_WORKER_OBJECT ( -57 + MDP_DEFINE )			//��Ч�ķ���������
#define ERR_APPEND_REQ_FAIL				( -58 + MDP_DEFINE )			//��ӵ��������βʧ��
#define ERR_DATA_ILLEGAL					( -59 + MDP_DEFINE )			//���ݲ��Ϸ������ݰ�С��2[������+����],��
#define ERR_APPEND_WAIT_FAIL			( -60 + MDP_DEFINE )			//��ӵ��ȴ������б�βʧ��
#define ERR_INSERT_WORKER_FAIL		( -61 + MDP_DEFINE )			//��ӵ���ϣ��Ĺ���ʧ��
#define ERR_INSERT_SERVER_FAIL		( -62 + MDP_DEFINE )			//��ӵ���ϣ��Ĺ���ʧ��
#define ERR_INVALID_SERVER_OBJECT ( -63 + MDP_DEFINE )

//����ṹ��
typedef struct {
	void *ctx;                  //  ������
	void *socket;               //  �ͻ��˺ͷ�����׽���
	int verbose;                //  ��ӡ
	char *endpoint;             //  ����󶨶˿�
	zhash_t *services;          //  �����������
	zhash_t *workers;           //  ��������
	zlist_t *waiting;           //  �ȴ���������
	uint64_t heartbeat_at;      //  �źż��ʱ��
} broker_t;

static broker_t * s_broker_new(int verbose ,int *err);
static int s_broker_destroy(broker_t **self_p);
static int s_broker_bind(broker_t *self, char *endpoint);
static int s_broker_worker_msg(broker_t *self, zframe_t *sender, zmsg_t *msg);
static int s_broker_client_msg(broker_t *self, zframe_t *sender, zmsg_t *msg);
static int s_broker_purge(broker_t *self);

//�������ṹ��
typedef struct {
	broker_t *broker;           //  ����ʵ��
	char *name;                 //  ������
	zlist_t *requests;          //  �ͻ��������б�
	zlist_t *waiting;           //  �ȴ��������б�
	size_t workers;             //  ��������
} service_t;

static service_t *s_service_require(broker_t *self, zframe_t *service_frame, int *err);
static void s_service_destroy(void *argument);
static int s_service_dispatch(service_t *service, zmsg_t *msg);

//  ����ṹ��

typedef struct {
	broker_t *broker;           //  ����ʵ��
	char *id_string;            //  �������ID
	zframe_t *identity;         //  ·�ɵ����֡
	service_t *service;         //  ���ڷ���
	int64_t expiry;             //  �źż�ⳬʱʱ��
} worker_t;

static worker_t *s_worker_require(broker_t *self, zframe_t *identity,int *err);
static int s_worker_delete(worker_t *self, int disconnect);
static void s_worker_destroy(void *argument);
static int s_worker_send(worker_t *self, char *command, char *option,zmsg_t *msg);
static int s_worker_waiting(worker_t *self);