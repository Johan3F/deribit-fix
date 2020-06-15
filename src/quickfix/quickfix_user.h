#pragma once

#include "../config.h"

namespace FIX {

/**
 * Interface for a user of the quickfix class to receive the messages that the
 * engine recieves
 *
 * @Note: This class methods won't have them pure virtual since the user might
 * want to only get some messages while not being interested in the others
 */
class quickfix_user {
 public:
  virtual void on_logon() {}
  virtual void on_logout() {}
  virtual void on_mass_status_report(int const /*report_number*/){};
  virtual void on_message(optional<positions_list_t> const& /*positions*/) {}
  virtual void on_message(
      optional<instruments_list_t> const& /*instruments*/){};
  virtual void on_message(execution_report_t& /*report*/){};
  virtual void on_message(market_update_t const& /*update*/){};
  virtual void on_message(mass_cancel_report_t const& /*report*/){};
  virtual void on_message(order_cancel_reject_t const& /*report*/){};
  virtual void on_message(string const& /*message*/){};
};

}  // namespace FIX
