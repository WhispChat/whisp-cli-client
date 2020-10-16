#include "whisp-cli/message.h"

#include <algorithm>
#include <cctype>
#include <google/protobuf/any.pb.h>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

Message::Message(std::string message) : message(message) {
  if (this->message.front() == '/') {
    is_command = true;

    // message is command, split message by space and save parts
    split_message_parts();
  }
}

void Message::split_message_parts() {
  std::istringstream iss(message);
  std::vector<std::string> result{std::istream_iterator<std::string>(iss), {}};
  message_parts = result;
}

std::string Message::get_message_str() {
  google::protobuf::Any any;

  if (is_command) {
    std::string command_type = message_parts.front();
    command_type.erase(0, 1);
    std::transform(command_type.begin(), command_type.end(),
                   command_type.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    std::vector<std::string> args = {message_parts.begin() + 1,
                                     message_parts.end()};

    client::Command cmd;

    cmd.set_type(command_type);
    for (auto arg : args) {
      cmd.add_args(arg);
    }

    any.PackFrom(cmd);
  } else {
    client::Message msg;
    msg.set_content(message);

    any.PackFrom(msg);
  }

  std::string msg_str;
  any.SerializeToString(&msg_str);

  return msg_str;
}
