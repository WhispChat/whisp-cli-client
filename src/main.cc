#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#elif _WIN32
#define NTDDI_VERSION NTDDI_VISTA
#define WINVER _WIN32_WINNT_VISTA
#define _WIN32_WINNT _WIN32_WINNT_VISTA

#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <cstring>
#include <google/protobuf/any.pb.h>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <strings.h>
#include <thread>

#include "whisp-cli/encryption.h"
#include "whisp-protobuf/cpp/client.pb.h"
#include "whisp-protobuf/cpp/server.pb.h"

// TODO: make configurable
const int SERVER_PORT = 8080;
const std::string SERVER_HOST = "127.0.0.1";

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

void read_server(int sock_fd) {
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
          std::cout << "[" << user_msg.registered_user().username()
                    << "]: " << user_msg.content() << '\n';
        } else if (user_msg.has_guest_user()) {
          std::cout << "[" << user_msg.guest_user().username()
                    << " (guest)]: " << user_msg.content() << '\n';
        }
      } else if (any.Is<server::Status>()) {
        server::Status status;
        any.UnpackTo(&status);

        if (status.number_connections() >= status.max_connections()) {
          print_message(server::Message::ERROR)
              << "Failed to join: Server full (" << status.max_connections()
              << "/" << status.max_connections() << ")\n";
          close(sock_fd);
          exit(EXIT_FAILURE);
        }

        print_message(server::Message::INFO) << "Connected to " << SERVER_HOST
                                             << ":" << SERVER_PORT << '\n';
        print_message(server::Message::INFO)
            << "Number of connected users: "
            << std::to_string(status.number_connections() + 1) << "/"
            << std::to_string(status.max_connections()) << '\n';

      } else if (any.Is<server::Message>()) {
        server::Message msg;
        any.UnpackTo(&msg);

        print_message(msg.type()) << msg.content() << '\n';
      } else if (any.Is<server::ServerClosed>()) {
        print_message(server::Message::INFO) << "Server closing.\n";
        close(sock_fd);
        exit(EXIT_SUCCESS);
      }
    }
    memset(buffer, 0, sizeof buffer);
  }
}

void prompt_user_input(int sock_fd) {
  std::string input;

  while (1) {
    std::getline(std::cin, input);

    client::Message msg;
    msg.set_content(input);
    std::string msg_str;
    msg.SerializeToString(&msg_str);

    std::string encrypted_input =
        Encryption::encrypt(msg_str, Encryption::OneTimePad);

    send(sock_fd, encrypted_input.data(), encrypted_input.size(), 0);

    std::cin.clear();
  }
}

int main(int argc, char **argv) {
  // verify that the version of the library that we linked against is
  // compatible with the version of the headers we compiled against.
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  int sock_fd;
  struct sockaddr_in serv_addr;

// Initialize Winsock
#ifdef _WIN32
  WSADATA wsaData;
  int err_code = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (err_code) {
    print_message(server::Message::ERROR)
        << "WSAStartup function failed with error code: " << err_code << "\n";
    return EXIT_FAILURE;
  }
#endif

  if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    print_message(server::Message::ERROR) << "Socket creation error\n";
#ifdef _WIN32
    WSACleanup();
#endif
    return EXIT_FAILURE;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(SERVER_PORT);

  // convert IP addresses from text to binary form
  if (inet_pton(AF_INET, SERVER_HOST.c_str(), &serv_addr.sin_addr) != 1) {
    print_message(server::Message::ERROR) << "Invalid IP address\n";
    return EXIT_FAILURE;
  }

  if (connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof serv_addr) == -1) {
    print_message(server::Message::ERROR) << "Couldn't connect to server\n";
    return EXIT_FAILURE;
  }

  // non-blocking receive from server in separate thread
  std::thread t(&read_server, sock_fd);
  t.detach();

  // block to read user input
  prompt_user_input(sock_fd);

  close(sock_fd);
#ifdef _WIN32
  WSACleanup();
#endif

  return EXIT_SUCCESS;
}
