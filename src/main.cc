#include "whisp-cli/config.h"
#include "whisp-cli/mainwindow.h"

int main(int argc, char **argv) {
  // Initialize configuration
  config::load();
  std::string server_host = config::read("SERVER_HOST");
  int server_port = std::stoi(config::read("SERVER_PORT"));

  QApplication app(argc, argv);
  MainWindow window(server_host, server_port);
  window.resize(740, 900);
  window.show();
  window.setWindowTitle(QApplication::translate("whisp", "Whisp Chat"));

  int res = app.exec();

  window.cleanup();

  return res;
}
