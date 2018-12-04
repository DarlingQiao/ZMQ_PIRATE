// testDll.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "zmq_mdp_server.h"
#include "parse_message.h"

#define PROXY_ADDR "tcp://localhost:5555"

int main(int argc, char* argv[])
{
#if 0
  _mdwrk_pub_t *session = server_pub_new(PROXY_ADDR);
  while (true) {
    server_pub_send(session,"A","hello world!");
    _sleep(1000);
  }
  server_pub_destroy(&session);
#endif

#if 1
  parse_message parse;
  mdwrk_t *session = server_new(PROXY_ADDR, "echo");

  while (true) {
    zmsg_t *request = server_recv(session);
    parse.parse_message_cmd(request);
    message_set(request, parse.outJson().c_str());
    server_reply(session, &request);
  }
  server_destroy(&session);
#endif
	return 0;
}

