#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <google/protobuf/any.pb.h>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <strings.h>
#include <gtkmm/application.h>
#include <thread>

#include "whisp-cli/encryption.h"
#include "whisp-protobuf/cpp/client.pb.h"
#include "whisp-cli/client_window.h"
#include "whisp-protobuf/cpp/server.pb.h"

// TODO: Make these settings configurable
const int SERVER_PORT = 8080;
const std::string SERVER_HOST = "0.0.0.0";

std::ostream &print_message(server::Message::MessageType type) {
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

int main(int argc, char **argv) {
  // verify that the version of the library that we linked against is
  // compatible with the version of the headers we compiled against.
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  int sock_fd;
  struct sockaddr_in serv_addr;

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(SERVER_PORT);

  if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    print_message(server::Message::ERROR) << "Socket creation error\n";
    return EXIT_FAILURE;
  }

  // convert IP addresses from text to binary form
  if (inet_pton(AF_INET, SERVER_HOST.c_str(), &serv_addr.sin_addr) != 1) {
    print_message(server::Message::ERROR) << "Invalid IP address\n";
    return EXIT_FAILURE;
  }

  // Initialize the GUI
  auto app = Gtk::Application::create(argc, argv, "whisp.client.interface");
  ClientWindow client_window;
  client_window.set_sock_fd(sock_fd);

  // Connect to the server
  client_window.add_message("Connecting to " + SERVER_HOST + ":" +
                            std::to_string(SERVER_PORT) + "...");
  print_message(server::Message::INFO) << "Connecting to " << SERVER_HOST << ":"
                                       << SERVER_PORT << "...\n";
  if (connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof serv_addr) == -1) {
    client_window.add_message("Couldn't connect to server.");
    print_message(server::Message::ERROR) << "Couldn't connect to server\n";
  } else {
    client_window.add_message("Connected successfully!");
    print_message(server::Message::ERROR) << "Connected successfully!\n";
  }

  // Non-blocking receive from server in separate thread
  std::thread t(&ClientWindow::read_server, &client_window);
  t.detach();

  // TODO: Figure out how to catch the window closing event so we can correctly
  //  close the socket
  // close(sock_fd);

  return app->run(client_window);
}