#include <arpa/inet.h>
#include <iostream>
#include <sstream>
#include <string>
#include <strings.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#include "whisp-cli/encryption.h"

// TODO: make configurable
const int SERVER_PORT = 8080;
const std::string SERVER_HOST = "0.0.0.0";

void read_server(int sock_fd) {
  char buffer[4096];

  while (1) {
    // TODO: more C++ way of reading to buffer using iostream?
    read(sock_fd, buffer, sizeof buffer);
    // Buffer is split because TCP packets may contain more than one message
    std::istringstream iss{buffer};
    std::string part;
    while (std::getline(iss, part, (char)23)) {
      std::cout << Encryption::decrypt(part, Encryption::OneTimePad)
                << std::endl;
    }
    bzero(buffer, sizeof buffer);
  }
}

void prompt_user_input(int sock_fd) {
  std::string input;

  while (1) {
    std::getline(std::cin, input);
    std::string encrypted_input =
        Encryption::encrypt(input, Encryption::OneTimePad);
    send(sock_fd, encrypted_input.data(), encrypted_input.size(), 0);
    std::cin.clear();
  }
}

int main(int argc, char **argv) {
  int sock_fd;
  struct sockaddr_in serv_addr;

  if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    std::cout << "socket creation error" << std::endl;
    return -1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(SERVER_PORT);

  // convert IP addresses from text to binary form
  if (inet_pton(AF_INET, SERVER_HOST.c_str(), &serv_addr.sin_addr) != 1) {
    std::cout << "invalid address" << std::endl;
    return -1;
  }

  if (connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof serv_addr) == -1) {
    std::cout << "connection failed" << std::endl;
    return -1;
  }

  std::cout << "[INFO] Connected to " << SERVER_HOST << ":" << SERVER_PORT
            << std::endl;

  // non-blocking receive from server in separate thread
  std::thread t(&read_server, sock_fd);
  t.detach();

  // block to read user input
  prompt_user_input(sock_fd);

  close(sock_fd);

  return 0;
}
