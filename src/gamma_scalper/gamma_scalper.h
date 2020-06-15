#pragma once

#include "../config.h"

#include "../quickfix/quickfix.h"

#include "levels.h"

#include <condition_variable>
#include <deque>
#include <mutex>
#include <unordered_set>

class gamma_scalper : public FIX::quickfix_user {
  struct position_info {
    position_t position;
    instrument_t instrument;
  };
  using positions_t = std::unordered_map<string, position_info>;

 public:
  /** Constructor */
  gamma_scalper(config_file_t &configuration);

  /** Destructor */
  ~gamma_scalper();

  /** Run the strategy */
  bool run();

  // Overriden methods of the quickfix_user interface
  virtual void on_logon() override;
  virtual void on_logout() override;
  virtual void on_mass_status_report(int const report_number) override;
  virtual void on_message(
      optional<instruments_list_t> const &positions) override;
  virtual void on_message(optional<positions_list_t> const &positions) override;
  virtual void on_message(execution_report_t &report) override;
  virtual void on_message(market_update_t const &update);

 private:
  // Prints a report of the current positions and open orders
  void print_report();

  // Evaluates market data, position and orders in the market
  void evaluate();

  // Updates delta values for the given position
  optional<string> update_deltas(double time_to_expiration);

  // Reports and exits the program with an exception
  void report_error(string const &message);

  // Cancel the current strategy order
  void cancel_all_orders();

  // Update the position based on an execution report
  void update_position(execution_report_t &report);

  // Configuration file to use
  config_file_t &m_config_file;

  // Control the reconnections and the main loop
  std::mutex m_mutex;
  std::condition_variable m_condition_variable;
  bool m_is_running;

  // Market access
  std::unique_ptr<FIX::quickfix> m_market;

  // Strategy's position
  positions_t m_positions;

  // Instruments to use. Straddle and future
  optional<instrument_t> m_straddle_call;
  optional<instrument_t> m_straddle_put;
  optional<instrument_t> m_future;

  // Storing information about the last buys and sells
  levels m_levels;

  // Market data snapshots done
  std::unordered_set<string> m_snapshots;

  // Deltas for the straddle and the future
  double m_delta_future;
  double m_delta_call;
  double m_delta_put;

  // Bid and ask orders open for this strategy
  optional<order_t> m_order;

  // Number of incoming execution report that are part of the mass status report
  int m_mass_reports_incoming;

  // Interest rates
  double m_interest_rate;
};
