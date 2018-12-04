#include "stdafx.h"
#include "operating_device.h"
#include "zmq_mdp_server.h"



operating_device::operating_device()
{
 
}


operating_device::~operating_device()
{
}

void out_error_code(std::system_error &e)
{
  std::cout << "Exception caught (system_error):\n";
  std::cout << "Error: " << e.what() << '\n';
  std::cout << "Code: " << e.code().value() << '\n';
  std::cout << "Category: " << e.code().category().name() << '\n';
  std::cout << "Message: " << e.code().message() << '\n';
}

void operating_device::resetJson(Json::Value &root,std::string cmd, 
  std::string error,std::string message) {

  root["cmd"] = cmd;
  root["error"] = error;
  root["message"] = message;

}

void operating_device::open(Json::Value &root) {

  std::string value = "0";
  std::string message = "success";

  std::string device = root["device"].asString();

  try{
    scanner.open(device);
  }
  catch (std::system_error &e){
    out_error_code(e);
    value = e.code().value();
    message = e.code().message();
  }
  root.clear();
  resetJson(root, "open", value, message);

}

void operating_device::close(Json::Value &root) {
  
  std::string value = "0";
  std::string message = "success";
  try{
    scanner.close();
  }
  catch (std::system_error &e){
    out_error_code(e);
    value = e.code().value();
    message = e.code().message();
  }
  root.clear();
  resetJson(root, "close", value, message);
}

void operating_device::enable(Json::Value &root) {
  
  std::string value = "0";
  std::string message = "success";
  try{
    scanner.enable();
  }
  catch (std::system_error &e){
    out_error_code(e);
    value = e.code().value();
    message = e.code().message();
  }
  root.clear();
  resetJson(root, "enable", value, message);
}

void operating_device::disable(Json::Value &root) {
 
  std::string value = "0";
  std::string message = "success";
  try{
    scanner.disable();
  }
  catch (std::system_error &e){
    out_error_code(e);
    value = e.code().value();
    message = e.code().message();
  }
  root.clear();
  resetJson(root, "disable", value, message);
}

void operating_device::get_barcode(Json::Value &root) {

  std::string value = "0";
  std::string message = "success";
  std::string barcode;
  std::error_code ec;
  try{
    _sleep(3000);
   barcode = scanner.get_barcode(ec);
  }
  catch (std::system_error &e){
    out_error_code(e);
    value = e.code().value();
    message = e.code().message();
  }
  root.clear();
  root["barcode"] = barcode;
  resetJson(root, "get_barcode", value, message);

  int num = 5;
  _mdwrk_pub_t *session = server_pub_new("tcp://localhost:5558");
  while (num) {
    server_pub_send(session, "A", "来自发布者的消息：获取条形码失败！");
    _sleep(100);
    num--;
  }
  server_pub_destroy(&session);
  printf("发布消息结束\n");
}

void operating_device::reset(Json::Value &root) {

  std::string value = "0";
  std::string message = "success";
  try{
   scanner.reset();
  }
  catch (std::system_error &e){
    out_error_code(e);
    value = e.code().value();
    message = e.code().message();
  }
  root.clear();
  resetJson(root, "reset", value, message);
}