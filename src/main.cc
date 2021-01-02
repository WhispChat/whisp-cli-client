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
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <strings.h>
#include <thread>

#include "whisp-cli/config.h"
#include "whisp-cli/encryption.h"
#include "whisp-cli/logging.h"
#include "whisp-protobuf/cpp/client.pb.h"
#include "whisp-protobuf/cpp/server.pb.h"

void cleanup(SSL *ssl, SSL_CTX *ssl_ctx, int sock_fd) {
  SSL_free(ssl);
  close(sock_fd);
  SSL_CTX_free(ssl_ctx);

#ifdef _WIN32
  WSACleanup();
#endif
}

void read_server(SSL *ssl, SSL_CTX *ssl_ctx, int sock_fd, std::string host, int port) {
  char buffer[4096];

  while (SSL_read(ssl, buffer, sizeof buffer) > 0) {
    std::string message(buffer);
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
        LOG_ERROR << "Failed to join: Server full (" << status.max_connections()
                  << "/" << status.max_connections() << ")\n";

        cleanup(ssl, ssl_ctx, sock_fd);
        exit(EXIT_FAILURE);
      }

      LOG_INFO << "Connected to " << host << ":" << port << '\n';
      LOG_INFO << "Number of connected users: "
               << std::to_string(status.number_connections() + 1) << "/"
               << std::to_string(status.max_connections()) << '\n';

    } else if (any.Is<server::Message>()) {
      server::Message msg;
      any.UnpackTo(&msg);

      switch (msg.type()) {
      case server::Message::INFO:
        LOG_INFO << msg.content() << '\n';
        break;
      case server::Message::ERROR:
        LOG_ERROR << msg.content() << '\n';
        break;
      case server::Message::DEBUG:
        LOG_DEBUG << msg.content() << '\n';
        break;
      }
    } else if (any.Is<server::ServerClosed>()) {
      LOG_INFO << "Server closing.\n";

      cleanup(ssl, ssl_ctx, sock_fd);
      exit(EXIT_SUCCESS);
    }
    memset(buffer, 0, sizeof buffer);
  }
}

SSL_CTX *initialize_ssl_context() {
  const SSL_METHOD *method = TLS_client_method();
  SSL_CTX *ssl_ctx = SSL_CTX_new(method);

  if (SSL_CTX_set_default_verify_paths(ssl_ctx) != 1) {
    std::cerr << "Error loading trust store\n";
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
  }

  SSL_CTX_set_min_proto_version(ssl_ctx, TLS1_2_VERSION);

  return ssl_ctx;
}

void prompt_user_input(SSL *ssl) {
  std::string input;

  while (1) {
    std::getline(std::cin, input);

    client::Message msg;
    msg.set_content(input);
    std::string msg_str;
    msg.SerializeToString(&msg_str);

    SSL_write(ssl, msg_str.data(), msg_str.size());

    std::cin.clear();
  }
}

int main(int argc, char **argv) {
  // Initialize configuration
  config::load();
  std::string server_host = config::read("SERVER_HOST");
  int server_port = std::stoi(config::read("SERVER_PORT"));

  // verify that the version of the library that we linked against is
  // compatible with the version of the headers we compiled against.
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  SSL_CTX *ssl_ctx = initialize_ssl_context();
  SSL *ssl = SSL_new(ssl_ctx);
  if (!ssl) {
    std::cerr << "SSL_new() failed\n";
    return EXIT_FAILURE;
  }

  int sock_fd;
  struct sockaddr_in serv_addr;

// Initialize Winsock
#ifdef _WIN32
  WSADATA wsaData;
  int err_code = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (err_code) {
    LOG_ERROR << "WSAStartup function failed with error code: " << err_code
              << "\n";
    return EXIT_FAILURE;
  }
#endif

  if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    LOG_ERROR << "Socket creation error\n";
#ifdef _WIN32
    WSACleanup();
#endif
    return EXIT_FAILURE;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(server_port);

  // convert IP addresses from text to binary form
  if (inet_pton(AF_INET, server_host.c_str(), &serv_addr.sin_addr) != 1) {
    LOG_ERROR << "Invalid IP address\n";
    return EXIT_FAILURE;
  }

  if (connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof serv_addr) == -1) {
    LOG_ERROR << "Couldn't connect to server\n";
    return EXIT_FAILURE;
  }

  SSL_set_fd(ssl, sock_fd);
  int status = SSL_connect(ssl);
  if (status != 1) {
    SSL_get_error(ssl, status);
    ERR_print_errors_fp(stderr);
    std::cerr << "SSL_connect failed with SSL_get_error code " << status
              << '\n';
    return EXIT_FAILURE;
  }

  // non-blocking receive from server in separate thread
  std::thread t(&read_server, ssl, ssl_ctx, sock_fd, server_host, server_port);
  t.detach();

  // block to read user input
  prompt_user_input(ssl);

  cleanup(ssl, ssl_ctx, sock_fd);

  return EXIT_SUCCESS;
}
