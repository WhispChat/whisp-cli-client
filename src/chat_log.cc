#include <gtkmm.h>
#include <string>

#include "whisp-cli/chat_log.h"

ChatLog::ChatLog() {
  messages_text_view.set_size_request(900, 400);
  messages_text_view.set_editable(false);

  add(messages_text_view);

  show_all_children();
}

void ChatLog::insert_message(const std::string& message) {
  auto chat_log_text_buffer = messages_text_view.get_buffer();

  Gtk::TextBuffer::iterator text_iterator = chat_log_text_buffer->get_iter_at_offset(-1);
  chat_log_text_buffer->insert(text_iterator, message);
}