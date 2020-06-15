#pragma once

#include "../config.h"
#include "quickfix_user.h"

#include <fstream>
#include <unordered_map>

#include <quickfix/Application.h>
#include <quickfix/Field.h>
#include <quickfix/MessageCracker.h>

#include <quickfix/FileLog.h>
#include <quickfix/FileStore.h>
#include <quickfix/SessionSettings.h>
#include <quickfix/SocketInitiator.h>

namespace FIX {

// Custom fields
USER_DEFINE_INT(DeribitTradeAmount, 100007);
USER_DEFINE_INT(DeribitSinceTimestamp, 100008);
USER_DEFINE_INT(DeribitTradeId, 100009);
USER_DEFINE_STRING(DeribitLabel, 100010);
USER_DEFINE_STRING(DeribitTotalPl, 100011);
USER_DEFINE_QTY(TradeVolume24h, 100087);
USER_DEFINE_INT(DeribitLiquidationPrice, 100088);
USER_DEFINE_STRING(DeribitBTCSize, 100089);
USER_DEFINE_PRICE(DeribitMarkPrice, 100090);
USER_DEFINE_PRICE(DeribitOpenInterest, 100091);

class quickfix : public Application, public FIX44::MessageCracker {
 public:
  /** Destructor */
  ~quickfix();

  /** Contructor */
  quickfix(config_file_t &config_file, quickfix_user &user);

  // Run and pause
  bool run();
  void stop();

  /** Implementing Application interface */
  void onCreate(SessionID const &) override;
  void onLogon(SessionID const &) override;
  void onLogout(SessionID const &) override;
  void toAdmin(Message &, SessionID const &) override;
  void toApp(Message &, SessionID const &) throw(DoNotSend) override;
  void fromAdmin(Message const &,
                 SessionID const &) throw(FieldNotFound, IncorrectDataFormat,
                                          IncorrectTagValue,
                                          RejectLogon) override;
  void fromApp(Message const &,
               SessionID const &) throw(FieldNotFound, IncorrectDataFormat,
                                        IncorrectTagValue,
                                        UnsupportedMessageType) override;

  // Methods for sending messages via quickfix
  void test_request();
  void request_instrument_list();
  void request_positions();
  void request_mass_status();
  void request_market_data(string const &symbol);
  string send_ioc_order(string const &symbol, side_t side, price_t order_price,
                        volume_t order_volume);
  string send_gtc_order(string const &symbol, side_t side, price_t order_price,
                        volume_t order_volume);
  void send_cancel_order(std::string const &order_to_cancel);
  // TODO: refine from here
  void send_single_order(string const &symbol);
  void send_mass_cancellation_order();
  void user_request();

 private:
  // Create a logon message
  void create_logon_message(Message &message);

  // OnMessage for all messages. This comes from the message cracker inheritance
  virtual void onMessage(FIX44::PositionReport const &message,
                         SessionID const &session_id) override;
  virtual void onMessage(FIX44::SecurityList const &message,
                         SessionID const &session_id) override;
  virtual void onMessage(FIX44::MarketDataRequestReject const &message,
                         SessionID const &session_id) override;
  virtual void onMessage(FIX44::MarketDataSnapshotFullRefresh const &message,
                         SessionID const &session_id) override;
  virtual void onMessage(FIX44::MarketDataIncrementalRefresh const &message,
                         SessionID const &session_id) override;
  virtual void onMessage(FIX44::ExecutionReport const &message,
                         SessionID const &session_id) override;
  virtual void onMessage(FIX44::OrderCancelReject const &message,
                         SessionID const &session_id) override;
  virtual void onMessage(FIX44::OrderMassCancelReport const &message,
                         SessionID const &session_id) override;

  // Encapsulates the sending of messages
  void send_message(Message &message, SessionID const &session);

  // Session identifier
  SessionID m_session_id;

  // User that will be using quickfix connection
  quickfix_user &m_user;

  // Next request identifier
  long m_request_identifier;

  // Next order identifier
  long m_order_identifier;

  // Configuration file
  config_file_t &m_configuration;

  // Specifics for quickfix
  FIX::Initiator *m_initiator;
  std::unique_ptr<FIX::SessionSettings> m_quickfix_settings;
  std::unique_ptr<FIX::SynchronizedApplication> m_quickfix_synch;
  std::unique_ptr<FIX::FileStoreFactory> m_quickfix_store_factory;
  std::unique_ptr<FIX::FileLogFactory> m_quickfix_log_factory;

  // Flag to know if this will be used to replaying logs or not
  bool m_log_replay;
};

}  // namespace FIX
