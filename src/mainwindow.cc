#include <google/protobuf/any.pb.h>
#include <google/protobuf/util/time_util.h>
#include <iostream>
#include <sstream>
#include <thread>

#include "whisp-cli/logging.h"
#include "whisp-cli/mainwindow.h"

MainWindow::MainWindow(std::string server_host, int server_port) {
  this->server_host = server_host;
  this->server_port = server_port;

  initialize_ssl();
  initialize_socket();
  initialize_layout();

  // non-blocking receive from server in separate thread
  std::thread t(&MainWindow::read_server, this);
  t.detach();
}

void MainWindow::initialize_ssl() {
  const SSL_METHOD *method = TLS_client_method();
  SSL_CTX *ssl_ctx = SSL_CTX_new(method);

  if (SSL_CTX_set_default_verify_paths(ssl_ctx) != 1) {
    std::cerr << "Error loading trust store\n";
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
  }

  SSL_CTX_set_min_proto_version(ssl_ctx, TLS1_2_VERSION);

  this->ssl_ctx = ssl_ctx;
  this->ssl = SSL_new(ssl_ctx);

  if (!this->ssl) {
    std::cerr << "SSL_new() failed\n";
    exit(EXIT_FAILURE);
  }
}

void MainWindow::initialize_socket() {
// Initialize Winsock
#ifdef _WIN32
  WSADATA wsaData;
  int err_code = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (err_code) {
    LOG_ERROR << "WSAStartup function failed with error code: " << err_code
              << "\n";
    exit(EXIT_FAILURE);
  }
#endif

  if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    LOG_ERROR << "Socket creation error\n";
#ifdef _WIN32
    WSACleanup();
#endif
    exit(EXIT_FAILURE);
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(server_port);

  // convert IP addresses from text to binary form
  if (inet_pton(AF_INET, server_host.c_str(), &serv_addr.sin_addr) != 1) {
    LOG_ERROR << "Invalid IP address\n";
    exit(EXIT_FAILURE);
  }

  if (::connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof serv_addr) ==
      -1) {
    LOG_ERROR << "Couldn't connect to server\n";
    exit(EXIT_FAILURE);
  }

  SSL_set_fd(ssl, sock_fd);
  int status = SSL_connect(ssl);
  if (status != 1) {
    SSL_get_error(ssl, status);
    ERR_print_errors_fp(stderr);
    std::cerr << "SSL_connect failed with SSL_get_error code " << status
              << '\n';
    exit(EXIT_FAILURE);
  }
}

void MainWindow::initialize_layout() {
  layout = new QGridLayout;

  // chat body
  chat_text = new QTextEdit;
  chat_text->resize(width(), height());
  chat_text->setReadOnly(true);
  chat_text->setAlignment(Qt::AlignTop);

  layout->addWidget(chat_text);

  // chat input box
  QHBoxLayout *chat_input_layout = new QHBoxLayout;
  QGroupBox *chat_input = new QGroupBox;

  chat_input_box = new QLineEdit;
  chat_input_box->resize(width(), chat_input->height());

  chat_input_button = new QPushButton("Send", this);
  connect(chat_input_button, &QPushButton::released, this,
          &MainWindow::send_message);
  connect(chat_input_box, &QLineEdit::returnPressed, chat_input_button,
          &QPushButton::click);

  chat_input_layout->addWidget(chat_input_box);
  chat_input_layout->addWidget(chat_input_button);
  chat_input->setLayout(chat_input_layout);

  layout->addWidget(chat_input);

  setLayout(layout);
}

void MainWindow::show_new_message(std::string message) {
  io_mutex.lock();
  QString qstr = QString::fromStdString(message);
  chat_text->append(qstr);
  io_mutex.unlock();
}

void MainWindow::send_message() {
  std::string content = chat_input_box->text().toStdString();

  if (content.compare("") == 0) {
    return;
  }

  client::Message msg;
  msg.set_content(content);

  std::string msg_str;
  msg.SerializeToString(&msg_str);

  SSL_write(ssl, msg_str.data(), msg_str.size());

  chat_input_box->setText("");
}

std::string MainWindow::format_message(client::Message user_msg) {
  std::stringstream msg_str;

  if (user_msg.has_registered_user()) {
    msg_str << "[<strong>" << user_msg.registered_user().username()
            << "</strong>]: " << user_msg.content();
  } else if (user_msg.has_guest_user()) {
    msg_str << "[" << user_msg.guest_user().username()
            << " (guest)]: " << user_msg.content();
  }

  return msg_str.str();
}

std::string MainWindow::format_message(user::PrivateMessageIn private_msg) {
  std::stringstream msg_str;

  msg_str << "<p style=\"color: #707070\">";
  if (private_msg.has_registered_user()) {
    msg_str << "[<strong>" << private_msg.registered_user().username()
            << "</strong>] whispers: " << private_msg.content();
  } else if (private_msg.has_guest_user()) {
    msg_str << "[" << private_msg.guest_user().username()
            << " (guest)] whispers: " << private_msg.content();
  }
  msg_str << "</p>";

  return msg_str.str();
}

std::string MainWindow::format_message(user::PrivateMessageOut private_msg) {
  std::stringstream msg_str;

  msg_str << "<p style=\"color: #707070\">";
  if (private_msg.has_registered_user()) {
    msg_str << "you whisper to [<strong>"
            << private_msg.registered_user().username()
            << "</strong>]: " << private_msg.content();
  } else if (private_msg.has_guest_user()) {
    msg_str << "you whisper to [" << private_msg.guest_user().username()
            << " (guest)]: " << private_msg.content();
  }
  msg_str << "</p>";

  return msg_str.str();
}

std::string MainWindow::format_message(server::Message msg) {
  std::stringstream msg_str;

  msg_str << "<em>";
  switch (msg.type()) {
  case server::Message::INFO:
    msg_str << "[INFO ] ";
    break;
  case server::Message::ERROR:
    msg_str << "[ERROR] ";
    break;
  case server::Message::DEBUG:
    msg_str << "[DEBUG] ";
    break;
  }
  msg_str << msg.content();
  msg_str << "</em>";

  return msg_str.str();
}

void MainWindow::read_server() {
  char buffer[4096];

  while (SSL_read(ssl, buffer, sizeof buffer) > 0) {
    std::string message(buffer);
    google::protobuf::Any any;
    any.ParseFromString(message);

    std::stringstream chat_message;

    if (any.Is<client::Message>()) {
      client::Message user_msg;
      any.UnpackTo(&user_msg);

      chat_message << format_message(user_msg);
      show_new_message(chat_message.str());
    } else if (any.Is<user::PrivateMessageIn>()) {
      user::PrivateMessageIn private_msg;
      any.UnpackTo(&private_msg);

      chat_message << format_message(private_msg);
      show_new_message(chat_message.str());
    } else if (any.Is<user::PrivateMessageOut>()) {
      user::PrivateMessageOut private_msg;
      any.UnpackTo(&private_msg);

      chat_message << format_message(private_msg);
      show_new_message(chat_message.str());
    } else if (any.Is<server::Status>()) {
      server::Status status;
      any.UnpackTo(&status);

      if (status.number_connections() >= status.max_connections()) {
        chat_message << "Failed to join: Server full ("
                     << status.max_connections() << "/"
                     << status.max_connections() << ")";

        show_new_message(chat_message.str());
        return;
      }

      chat_message << "Connected to " << server_host << ":" << server_port
                   << '\n';
      chat_message << "Number of connected users: "
                   << std::to_string(status.number_connections() + 1) << "/"
                   << std::to_string(status.max_connections());
      show_new_message(chat_message.str());
    } else if (any.Is<server::Message>()) {
      server::Message msg;
      any.UnpackTo(&msg);

      chat_message << format_message(msg);
      show_new_message(chat_message.str());
    } else if (any.Is<server::ServerClosed>()) {
      chat_message << "<em>[INFO]"
                   << "Server closing.</em>";
      show_new_message(chat_message.str());
      cleanup();
      exit(EXIT_SUCCESS);
    }
    memset(buffer, 0, sizeof buffer);
  }
}

void MainWindow::cleanup() {
  SSL_free(ssl);
  ::close(sock_fd);
  SSL_CTX_free(ssl_ctx);

#ifdef _WIN32
  WSACleanup();
#endif
}
