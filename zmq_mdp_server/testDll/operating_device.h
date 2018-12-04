#ifndef __OPERATING_DEVICE_H__
#define __OPERATING_DEVICE_H__

#include "json.h"
#include "flatbed_scanner.h"
#include "io_com.h"
#include <iostream>
#include <string>
#include <system_error>

class operating_device
{
public:
  operating_device();
  ~operating_device();
public:
 void open(Json::Value &root);
 void reset(Json::Value &root);
 void close(Json::Value &root);
 void get_barcode(Json::Value &root);
 void enable(Json::Value &root);
 void disable(Json::Value &root);

private:
  flatbed_scanner scanner;
  Json::FastWriter write;

protected:
  void resetJson(Json::Value &root,std::string cmd, std::string error, std::string message);
};

#endif