#include <google/protobuf/any.pb.h>
#include <sys/socket.h>

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
  send_button.signal_clicked().connect(sigc::mem_fun(*this,
      &ClientWindow::send_input_to_server));

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

  std::string encrypted_input = Encryption::encrypt(message_string,
      Encryption::OneTimePad);
  send(ClientWindow::sock_fd, encrypted_input.data(), encrypted_input.size(),
      0);
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

void ClientWindow::read_server() {
  char buffer[4096];

  while (1) {
    recv(sock_fd, buffer, sizeof buffer, 0);

    // Buffer is split because TCP packets may contain more than one message
    std::istringstream iss{buffer};
    std::string part;
    while (std::getline(iss, part, (char)23)) {
      std::string message = Encryption::decrypt(part, Encryption::OneTimePad);
      google::protobuf::Any any;
      any.ParseFromString(message);

      if (any.Is<client::Message>()) {
        client::Message user_msg;
        any.UnpackTo(&user_msg);

        if (user_msg.has_registered_user()) {
          ClientWindow::add_message("[" + user_msg.registered_user().username()
              + "]: " + user_msg.content());
          std::cout << "[" << user_msg.registered_user().username()
                    << "]: " << user_msg.content() << '\n';
        } else if (user_msg.has_guest_user()) {
          ClientWindow::add_message("[" + user_msg.guest_user().username()
              + "(guest)]: " + user_msg.content());
          std::cout << "[" << user_msg.guest_user().username()
                    << " (guest)]: " << user_msg.content() << '\n';
        }
      } else if (any.Is<server::Status>()) {
        server::Status status;
        any.UnpackTo(&status);

        if (status.number_connections() >= status.max_connections()) {
          ClientWindow::add_message("Failed to join: Server full ("
              + std::to_string(status.max_connections()) + "/"
              + std::to_string(status.max_connections()) + ")");
          print_message(server::Message::ERROR)
              << "Failed to join: Server full (" << status.max_connections()
              << "/" << status.max_connections() << ")\n";
          ::close(sock_fd);
          exit(EXIT_FAILURE);
        }

        ClientWindow::add_message("Connected to " + SERVER_HOST + ":"
            + std::to_string(SERVER_PORT));
        print_message(server::Message::INFO) << "Connected to " << SERVER_HOST
                                             << ":" << SERVER_PORT << '\n';
        ClientWindow::add_message("Number of connected users: "
            + std::to_string(status.number_connections() + 1) + "/"
            + std::to_string(status.max_connections()));
        print_message(server::Message::INFO)
            << "Number of connected users: "
            << std::to_string(status.number_connections() + 1) << "/"
            << std::to_string(status.max_connections()) << '\n';

      } else if (any.Is<server::Message>()) {
        server::Message msg;
        any.UnpackTo(&msg);

        print_message(msg.type()) << msg.content() << '\n';
      } else if (any.Is<server::ServerClosed>()) {
        ClientWindow::add_message("Server closing.");
        print_message(server::Message::INFO) << "Server closing.\n";
        ::close(sock_fd);
      }
    }
    memset(buffer, 0, sizeof buffer);
  }
}

void ClientWindow::set_sock_fd(int new_sock_fd) {
  ClientWindow::sock_fd = new_sock_fd;
}
