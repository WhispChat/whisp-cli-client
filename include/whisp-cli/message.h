#pragma once

#include "whisp-protobuf/cpp/client.pb.h"

#include <vector>

class Message {
public:
  Message(std::string message);

  std::string get_message_str();

private:
  void split_message_parts();

  std::string message;
  bool is_command = false;
  std::vector<std::string> message_parts;
};
