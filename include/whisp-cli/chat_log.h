#pragma once

#include <gtkmm.h>
#include <string>

class ChatLog : public Gtk::ScrolledWindow {
public:
  ChatLog();
  void insert_message(const std::string& message);

private:
  Gtk::TextView messages_text_view;
};