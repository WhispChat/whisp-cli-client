#pragma once

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

#include <QtWidgets>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <sstream>

class MainWindow : public QWidget {
public:
  MainWindow(std::string server_host, int server_port);
  void cleanup();

private:
  void initialize_ssl();
  void initialize_socket();
  void initialize_layout();
  void show_new_message(std::string message);
  void send_message();
  void read_server();

  SSL_CTX *ssl_ctx;
  SSL *ssl;

  std::string server_host;
  int server_port;
  int sock_fd;
  struct sockaddr_in serv_addr;

  QGridLayout *layout;
  QTextEdit *chat_text;
  QLineEdit *chat_input_box;
  QPushButton *chat_input_button;
};
