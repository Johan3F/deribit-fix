#pragma once

#include "message_parser_helpers.h"
#include "quickfix.h"

#include <fstream>
#include <thread>

class quickfix_log_replayer {
 public:
  // Destructor
  ~quickfix_log_replayer(){};

  // Constructor
  quickfix_log_replayer(FIX::quickfix& owner, string const file_path,
                        FIX::DataDictionary const data_dictionary,
                        FIX::SessionID const session_id, string receiver)
      : m_owner(owner),
        m_file(file_path),
        m_fix_dictionary(data_dictionary),
        m_session_id(session_id),
        m_receiver(receiver){};

  /** Start the thread that will read the file */
  void start() {
    std::thread reader(&quickfix_log_replayer::process_file, this);
    reader.join();
  }

 private:
  // Reads the line and communicates to the owner when needed
  void process_file() {
    std::string line;
    while (std::getline(m_file, line)) {
      // Remove the timestamp
      line = line.substr(30);

      try {
        // Parse line
        FIX::Message msg(line, m_fix_dictionary, false);

        // Send to the owner only if it's for it
        auto sender =
            FIX::field<string>::get(msg.getHeader(), FIX::FIELD::SenderCompID);
        if (sender != m_receiver) {
          continue;
        }

        m_owner.fromApp(msg, m_session_id);
      } catch (FIX::InvalidMessage const& e) {
        std::cout << "Invalid message: " << e.what() << std::endl;
      } catch (FIX::UnsupportedMessageType const& e) {
        std::cout << "Unsuported message type: " << e.what() << std::endl;
      } catch (std::exception const& e) {
        std::cout << "Unsuported message type: " << e.what() << std::endl;
      }
    }
  }

  // class that will receive the callbacks
  FIX::quickfix& m_owner;

  // File from where to get the logs
  std::ifstream m_file;

  // Fix data dictionary
  FIX::DataDictionary m_fix_dictionary;

  // Session identifier
  FIX::SessionID m_session_id;

  // Receiver of the messages
  string m_receiver;
};
