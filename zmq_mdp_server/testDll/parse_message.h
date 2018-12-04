#ifndef __PARSE_MESSAGE_H__
#define __PARSE_MESSAGE_H__

#include "zmq_mdp_server.h"
#include "operating_device.h"
#include "json.h"

struct NAME_MAP {
  int code;
  char* str;
};

enum CTRLCODE {
  OPEN = 0,
  CLOSE,
  RESET,
  ENABLE,
  DISABLE,
  GET_BARCODE,
  UNKNOWN
};

class parse_message
{

public:
  parse_message();
  ~parse_message();

public:
  operating_device device;
  void parse_message_cmd(zmsg_t *request);

  inline std::string outJson() {
    return write.write(root);
  }

private:
  Json::Value root;
  Json::FastWriter write;
  CTRLCODE code;

};

#endif