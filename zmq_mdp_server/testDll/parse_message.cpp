#include "stdafx.h"
#include "parse_message.h"

NAME_MAP ctrlcode_map[UNKNOWN + 1] = {
    { OPEN, "open" },
    { CLOSE, "close" },
    { RESET, "reset" },
    { ENABLE, "enable" },
    { DISABLE, "disable" },
    { GET_BARCODE, "get_barcode" },
    { UNKNOWN, "unknow" }
};

parse_message::parse_message()
{

}

parse_message::~parse_message()
{
}

void unknown_cmd(Json::Value &root,std::string cmd) {
  root.clear();
  root["cmd"] = cmd;
  root["error"] = "unknown_code";
  root["message"] = "Can't find the command";
}

void parse_message::parse_message_cmd(zmsg_t *request) {
  
  if (request == NULL) {
    return;
  }

  Json::Reader read;

  zframe_t *data = zmsg_last(request);
  char *str = zframe_strdup(data);

  std::string buffer = str;

  //判断是否为标准json

  if (!read.parse(buffer, root)) {
    ;//错误
  }

  std::string cmd = root["cmd"].asString();
 
  code = UNKNOWN;
  for (int i = 0; i < UNKNOWN; i++) {

    if (strncmp(cmd.c_str(), ctrlcode_map[i].str, strlen(ctrlcode_map[i].str)) == 0) {
      code = (CTRLCODE)ctrlcode_map[i].code;
      break;
    }
  }
  switch (code)
  {
  case OPEN:
    device.open(root);
    break;
  case CLOSE :
    device.close(root);
    break;
  case RESET:
    device.reset(root);
    break;
  case ENABLE:
    device.enable(root);
    break;
  case DISABLE:
    device.disable(root);
    break;
  case GET_BARCODE:
    device.get_barcode(root);
    break;
  default:
    unknown_cmd(root,cmd);
    return;
  }

}
