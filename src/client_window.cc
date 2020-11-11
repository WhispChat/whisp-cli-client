#include <sys/socket.h>

#include <google/protobuf/any.pb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <sstream>

#include "whisp-cli/client_window.h"
#include "whisp-cli/encryption.h"
#include "whisp-protobuf/cpp/client.pb.h"

// TODO: Make these settings configurable
const int SERVER_PORT = 8080;
const std::string SERVER_HOST = "0.0.0.0";

ClientWindow::ClientWindow() {
  set_title("Whisp Client");
  set_default_size(900, 600);
  set_border_width(10);

  messages_list.set_size_request(900, 500);

  send_button.set_label("Send");
  send_button.signal_clicked().connect(sigc::mem_fun(*this, &ClientWindow::send_input_to_server));

  text_input_paned.set_position(550);
  text_input_paned.add1(text_box);
  text_input_paned.add2(send_button);

  root_paned.add1(messages_list);
  root_paned.add2(text_input_paned);

  add(root_paned);

  show_all_children();
}

void ClientWindow::add_message(const std::string& message) {
  messages_list.insert_message(message + "\n");
}

void ClientWindow::send_input_to_server() {
  std::string input_text = text_box.get_buffer()->get_text();

  client::Message message;
  message.set_content(input_text);
  text_box.get_buffer()->set_text("");

  std::string message_string;
  message.SerializeToString(&message_string);

  std::string encrypted_input = Encryption::encrypt(message_string, Encryption::OneTimePad);
  send(ClientWindow::sock_fd, encrypted_input.data(), encrypted_input.size(), 0);
}

std::ostream &ClientWindow::print_message(server::Message::MessageType type) {
  switch (type) {
    case server::Message::INFO:
      std::cout << "[INFO ] ";
      return std::cout;
    case server::Message::ERROR:
      std::cerr << "[ERROR] ";
      return std::cerr;
    case server::Message::DEBUG:
      std::cout << "[DEBUG] ";
      return std::cout;
  }
  return std::cout;
}

void ClientWindow::set_sock_fd(int new_sock_fd) {
  ClientWindow::sock_fd = new_sock_fd;
}
