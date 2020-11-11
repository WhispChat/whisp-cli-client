#pragma once

#include <gtkmm.h>

#include "chat_log.h"
#include "whisp-protobuf/cpp/server.pb.h"

class ClientWindow : public Gtk::Window {
public:
  ClientWindow();
  void add_message(const std::string& message);
  void set_sock_fd(int sock_fd);
  void read_server();

private:
  Gtk::VPaned root_paned;
  Gtk::HPaned text_input_paned;
  Gtk::Button send_button;
  Gtk::TextView text_box;
  ChatLog messages_list;
  int sock_fd;

  std::ostream &print_message(server::Message::MessageType type);
  void send_input_to_server();
};
