#include "gamma_scalper.h"
#include "../pricing/black_scholes.h"

#include <boost/bind.hpp>

#include <cmath>
#include <fstream>
#include <regex>

namespace details {

string const levels_file = "levels";
string const pnl_file = "pnl";
string const pnl_log_file = "pnl_log";

/**
 * @return mid price if bid and ask are present, bid or ask if one is
 * missing or none if no bbo is present*/
optional<price_t> get_price(optional<BBO_t> const &bbo) {
  // Empty bbo
  if (!bbo) {
    return optional<price_t>(boost::none);
  }
  // Missing any of the two
  if (!bbo->ask || !bbo->bid) {
    return optional<price_t>(boost::none);
  }

  // Both are present, getting mid
  // TODO: ponderating with the volume
  return static_cast<price_t>(static_cast<double>(*bbo->ask + *bbo->bid) * 0.5);
}

optional<price_t> get_call_price(optional<BBO_t> const &call_bbo,
                                 optional<BBO_t> const &put_bbo,
                                 optional<price_t> const &underlying_price,
                                 double time_to_expiration, double strike,
                                 double interest_rate) {
  // We have market, return the mid point
  auto const call_price = get_price(call_bbo);
  if (call_price) {
    return call_price;
  }
  // If we don't have the other side of the market either, price is empty
  auto const put_price = get_price(put_bbo);
  if (!put_price) {
    return boost::none;
  }

  // Applying the put-call-parity property: C-P=D(S-K)-> C=P+S-Ke^(-rT)
  auto const S = *underlying_price;
  auto const P = *put_price * S;  // Converting to USD
  auto const K = strike * std::exp(-interest_rate * time_to_expiration);

  return optional<price_t>((P + S - K) / S);  // Converting back to CCY
}

optional<price_t> get_put_price(optional<BBO_t> const &call_bbo,
                                optional<BBO_t> const &put_bbo,
                                optional<price_t> const &underlying_price,
                                double time_to_expiration, double strike,
                                double interest_rate) {
  // We have market, return the mid point
  auto const put_price = get_price(put_bbo);
  if (put_price) {
    return put_price;
  }
  // If we don't have the other side of the market either, price is empty
  auto const call_price = get_price(call_bbo);
  if (!call_price) {
    return call_price;
  }

  // Applying the put-call-parity property: C-P=D(S-K)-> P=C-S+Ke^(rT)
  auto const S = *underlying_price;
  auto const C = *call_price * S;  // Converting to USD
  auto const K = strike * std::exp(-interest_rate * time_to_expiration);

  return optional<price_t>((C - S + K) / S);  // Converting back to CCY
}

/**
 * @return the delta for the given option based on the black-scholes formula*/
optional<double> get_delta(option_type_t call_or_put, double stock_price,
                           double strike_price, double risk_free_interest,
                           double time_to_expiration, double cost_of_carry,
                           double raw_option_price) {
  // convert raw price into USD price
  double option_price = raw_option_price * stock_price;
  // Calculate implied volatility
  auto implied_volatility = black_scholes_implied_volatility(
      call_or_put, stock_price, strike_price, time_to_expiration,
      risk_free_interest, cost_of_carry, option_price);

  // If not possible to get the implied volatility, delta will be empty
  if (!implied_volatility) {
    return optional<double>(boost::none);
  }

  return black_scholes_delta(call_or_put, stock_price, strike_price,
                             risk_free_interest, time_to_expiration,
                             cost_of_carry, *implied_volatility);
}

/**
 * Updates filled volume in an order from an order and an execution report
 */
void update_filled_volume(order_t &order, execution_report_t &report) {
  auto const filled = *report.executed_volume - order.full_volume;
  order.full_volume = *report.executed_volume;
  *report.executed_volume = filled;
}

}  // namespace details

gamma_scalper::gamma_scalper(config_file_t &configuration)
    : m_config_file(configuration),
      m_mutex(),
      m_condition_variable(),
      m_is_running(),
      m_market(std::make_unique<FIX::quickfix>(configuration, *this)),
      m_positions(),
      m_straddle_call(),
      m_straddle_put(),
      m_future(),
      m_levels(configuration["AuxFolder"],
               boost::lexical_cast<double>(configuration["PriceSweetener"])),
      m_snapshots(),
      m_delta_future(0),
      m_delta_call(0),
      m_delta_put(0),
      m_order(),
      m_mass_reports_incoming(0),
      m_interest_rate() {
  m_interest_rate = boost::lexical_cast<double>(configuration["InterestRate"]);
}

gamma_scalper::~gamma_scalper() {
  if (m_market) {
    m_market->stop();
  }
}

bool gamma_scalper::run() {
  std::cout << "Running gamma scalper strategy..." << std::endl;
  m_is_running = true;
  if (!m_market->run()) {
    std::cerr << "ERROR: Impossible to initialize the market" << std::endl;
    return false;
  }

  // Stay waiting for user input until "exit" and print report when input comes
  std::unique_lock<std::mutex> lck(m_mutex);
  while (m_is_running) {
    m_condition_variable.wait(lck);
  }

  return true;
}

void gamma_scalper::on_logon() {
  // Susbscribe for positions feed
  m_market->request_positions();
}

void gamma_scalper::on_logout() {
  std::unique_lock<std::mutex> lck(m_mutex);
  m_is_running = false;
  m_condition_variable.notify_all();
}

void gamma_scalper::on_mass_status_report(int const report_number) {
  if (report_number > 1) {
    report_error(
        "We're expecting to have maximun 1 open orders. We are getting " +
        to_string(report_number) +
        " which is not allowed. Exiting before something goes wrong");
  }
  // No need to wait for execution reports. Open market
  m_mass_reports_incoming = report_number;
  if (report_number == 0) {
    m_market->request_market_data(m_future->symbol);
    m_market->request_market_data(m_straddle_call->symbol);
    m_market->request_market_data(m_straddle_put->symbol);
  }
}

void gamma_scalper::on_message(
    optional<instruments_list_t> const &instruments) {
  if (!instruments) {
    report_error(
        "No instruments where retrieved. This is not a what "
        "is supposed to happen. Exiting before something goes wrong");
    return;
  }

  // Filling position's instruments
  for (auto &position : m_positions) {
    auto it =
        std::find_if(instruments->begin(), instruments->end(),
                     boost::bind(&instrument_t::symbol, _1) == position.first);
    if (it == instruments->end()) {
      report_error(
          "There's no instrument information for instrument's position" +
          position.first + ". Exiting before something goes wrong");
    }

    position.second.instrument = *it;

    // Calculating what is the straddle and what is the future to use
    if (position.second.instrument.type == "OPT") {
      if (position.second.instrument.put_call.get() == option_type_t::CALL) {
        m_straddle_call = position.second.instrument;
      } else {
        m_straddle_put = position.second.instrument;
      }
    } else {
      m_future = position.second.instrument;
    }
  }

  // Checking that we have the straddle and that it's a valid one
  if (!m_straddle_call || !m_straddle_put) {
    report_error(
        "After getting the instrument list, impossible to determine the "
        "straddle. This should never happen!");
  }
  if ((m_straddle_call->main_currency != m_straddle_put->main_currency) ||
      (m_straddle_call->maturity_date != m_straddle_put->maturity_date) ||
      (m_straddle_call->strike_price != m_straddle_put->strike_price)) {
    report_error("The straddle is not correct. " + m_straddle_call->symbol +
                 " and " + m_straddle_put->symbol +
                 " are not allowed to be together");
  }

  // If we don't have the future, search for one that fits, else the
  // perpetual
  if (!m_future) {
    auto const future_symbol = m_straddle_call->symbol.substr(0, 11);
    auto fitting_future =
        std::find_if(instruments->begin(), instruments->end(),
                     boost::bind(&instrument_t::symbol, _1) == future_symbol);
    if (fitting_future != instruments->end()) {
      m_future = *fitting_future;
    } else {
      auto const perpetual_symbol = future_symbol.substr(0, 3) + "-PERPETUAL";
      auto perpetual = std::find_if(
          instruments->begin(), instruments->end(),
          boost::bind(&instrument_t::symbol, _1) == perpetual_symbol);
      if (perpetual == instruments->end()) {
        report_error("Impossible to find the Perpetual (" + perpetual_symbol +
                     "). Exiting before something wrong happens");
      }
      m_future = *perpetual;
    }

    // Create an empty position for the future
    m_positions[m_future->symbol].instrument = *m_future;
    m_positions[m_future->symbol].position = position_t{
        m_future->symbol, volume_t(0), side_t::BUY, price_t(0), price_t(0)};
  }

  // Requesting now all active orders (It could be that we have some hanging
  // from last run)
  m_market->request_mass_status();
}

void gamma_scalper::on_message(optional<positions_list_t> const &positions) {
  // Erase our information and get fresh new from market
  m_positions.clear();

  if (!positions) {
    report_error("No positions retrieved. Stopping strategy");
  }

  for (auto const &position : *positions) {
    m_positions[position.symbol].position = position;
  }

  m_market->request_instrument_list();
}

void gamma_scalper::on_message(execution_report_t &report) {
  // If we are waiting for report after requesting a mass status report
  if (m_mass_reports_incoming > 0) {
    // Store the execution report in the correct side
    m_order = order_t{};

    m_order->id = report.order_id.get();
    m_order->original_id = report.original_order_id.get();
    m_order->side = report.side.get();
    m_order->order_price = report.order_price.get();
    m_order->full_volume = report.executed_volume.get();
    m_order->open_volume = report.open_volume.get();

    --m_mass_reports_incoming;

    // All orders were received already
    if (m_mass_reports_incoming == 0) {
      m_market->request_market_data(m_future->symbol);
      m_market->request_market_data(m_straddle_call->symbol);
      m_market->request_market_data(m_straddle_put->symbol);
    }
    return;
  }

  // Normal processing of an execution report
  if (!report.symbol) {
    return;
  }

  // If it's an order sent by the strategy, check what to do
  if ((m_order && report.order_id && m_order->id == *report.order_id) ||
      (m_order && report.original_order_id &&
       m_order->original_id == *report.original_order_id)) {
    switch (*report.order_status) {
      case order_status_t::FILLED:
        details::update_filled_volume(*m_order, report);
        update_position(report);
      case order_status_t::CANCELED:
      case order_status_t::REJECTED:
        m_order = optional<order_t>(boost::none);
        break;
      case order_status_t::PARTIAL: {
        m_order->id = *report.order_id;
        details::update_filled_volume(*m_order, report);
        update_position(report);
        break;
      }
      case order_status_t::NEW:
        m_order->id = *report.order_id;
        break;
    }
  } else {
    // Order not sent by the strategy
    switch (*report.order_status) {
      case order_status_t::FILLED:
      case order_status_t::PARTIAL:
        update_position(report);
    }
  }
}

void gamma_scalper::on_message(market_update_t const &update) {
  // We are always expecting to get one bid and one ask
  if (update.updates.size() > 2) {
    report_error("Received an bbo with more than two legs. This is wrong");
  }

  BBO_t target_bbo;
  for (auto const &update_component : update.updates) {
    if (update_component.side == market_side_t::BID) {
      target_bbo.bid_volume = update_component.level_volume;
      target_bbo.bid = update_component.level_price;
    } else {
      target_bbo.ask_volume = update_component.level_volume;
      target_bbo.ask = update_component.level_price;
    }
  }

  if (update.symbol == m_future->symbol) {
    m_future->bbo = target_bbo;
  } else if (update.symbol == m_straddle_call->symbol) {
    m_straddle_call->bbo = target_bbo;
  } else if (update.symbol == m_straddle_put->symbol) {
    m_straddle_put->bbo = target_bbo;
  } else {
    // Ignore this update. Not for straddle nor underlying
    return;
  }

  // We're subscribing to three instruments, therefore, we wait for the
  // snapshots of each before considering the strategy to be initialized
  if (m_snapshots.size() < 3) {
    m_snapshots.emplace(update.symbol);
    // This double check will only happen three times
    if (m_snapshots.size() < 3) {
      return;
    }
  }

  evaluate();
}

void gamma_scalper::print_report() {
  std::cout << std::endl;
  std::cout << "############### Positions  #################" << std::endl;
  for (auto const &position : m_positions) {
    std::cout << position.second.position << std::endl;
  }
  std::cout << "+----------- Instruments to use -----------+" << std::endl;
  std::cout << "Straddle call: " << m_straddle_call << std::endl;
  std::cout << "Straddle put : " << m_straddle_put << std::endl;
  std::cout << "fufure       : " << m_future << std::endl;
  std::cout << "+--------- Straddle's strike price --------+" << std::endl;
  std::cout << to_string(m_straddle_call->strike_price) << std::endl;
  std::cout << "+--------------- Active order -------------+" << std::endl;
  std::cout << "- " << m_order << std::endl;
  std::cout << "+------------------- BBOs -----------------+" << std::endl;
  if (m_future) {
    std::cout << "future " << m_future->symbol << ": " << std::endl;
    if (m_future->bbo) {
      std::cout << to_string(m_future->bbo->bid_volume) << " # "
                << to_string(m_future->bbo->bid) << " - "
                << to_string(m_future->bbo->ask) << " # "
                << to_string(m_future->bbo->ask_volume) << std::endl;
    }
  }
  if (m_straddle_call) {
    std::cout << "call " << m_straddle_call->symbol << ": " << std::endl;
    if (m_straddle_call->bbo) {
      std::cout << to_string(m_straddle_call->bbo->bid_volume) << " # "
                << to_string(m_straddle_call->bbo->bid) << " - "
                << to_string(m_straddle_call->bbo->ask) << " # "
                << to_string(m_straddle_call->bbo->ask_volume) << std::endl;
    }
  }
  if (m_straddle_put) {
    std::cout << "put " << m_straddle_put->symbol << ": " << std::endl;
    if (m_straddle_put->bbo) {
      std::cout << to_string(m_straddle_put->bbo->bid_volume) << " # "
                << to_string(m_straddle_put->bbo->bid) << " - "
                << to_string(m_straddle_put->bbo->ask) << " # "
                << to_string(m_straddle_put->bbo->ask_volume) << std::endl;
    }
  }
  std::cout << "+------------------- Deltas ---------------+" << std::endl;
  std::cout << "future: " << m_delta_future << std::endl;
  std::cout << "call  : " << m_delta_call << std::endl;
  std::cout << "put   : " << m_delta_put << std::endl;
  std::cout << "############################################" << std::endl;
  std::cout << std::endl;
}

void gamma_scalper::evaluate() {
  std::cout << "gammas_scalper::evaluate" << std::endl;
  // Getting time to expiration
  ptime now = second_clock::local_time();
  double time_to_expiration =
      (m_straddle_call->maturity_date->date() - now.date()).days() / 360.0;

  if (time_to_expiration < 0) {
    report_error("Straddles maturity was reached, stopping strategy");
    BOOST_THROW_EXCEPTION(
        std::logic_error("Stopping because maturity was reached"));
  }

  // Updating deltas
  auto error = update_deltas(time_to_expiration);

  if (error) {
    std::cout << "Skipping: " << to_string(error) << std::endl;
    // If we can't calculate deltas, ignore this cicle
    return;
  }

  // Calculating if new orders are needed
  auto total_delta = m_delta_put + m_delta_call + m_delta_future;

  auto delta_per_future =
      *m_future->contract_multiplier /
      static_cast<double>(*details::get_price(m_future->bbo));

  int corrections_todo = static_cast<int>(
      (static_cast<int>(std::round(total_delta / delta_per_future)) /
       *m_future->contract_multiplier) *
      *m_future->contract_multiplier);

  std::cout << "Future delta     : " << m_delta_future << std::endl;
  std::cout << "Call delta       : " << m_delta_call << std::endl;
  std::cout << "Put  delta       : " << m_delta_put << std::endl;
  std::cout << "Total delta      : " << total_delta << std::endl;
  std::cout << "Delta per future : " << delta_per_future << std::endl;
  std::cout << "Corrections to do: " << corrections_todo << std::endl;

  if (corrections_todo != 0) {
    // Sending an order for correcting the position's delta
    auto side = corrections_todo < 0 ? side_t::BUY : side_t::SELL;
    std::cout << "side : " << to_string(side) << std::endl;

    // If there's an active order, don't send more if it's in the same
    // side. If it's opposite side, cancel the existing one and wait for the
    // next cicle to process
    if (m_order) {
      if (m_order->side != side) {
        std::cout << "Canceling previous order: " << to_string(m_order->id)
                  << std::endl;
        m_market->send_cancel_order(m_order->id);
      }
      return;
    }

    price_t price_to_use = m_levels.get_price_to_use(side, *m_future);
    volume_t volume_to_use =
        m_levels.get_volume_to_use(side, volume_t(std::abs(corrections_todo)));
    std::cout << "Price to use: " << to_string(price_to_use) << std::endl;
    std::cout << "Volume to use: " << to_string(volume_to_use) << std::endl;

    m_order = order_t{};
    auto const order_id =
        m_market->send_gtc_order(m_future->symbol, side, price_to_use,
                                 static_cast<volume_t>(volume_to_use));
    m_order->original_id = order_id;
    m_order->open_volume = volume_to_use;
    m_order->full_volume = 0;
    m_order->side = side;
    m_order->order_price = price_to_use;
    std::cout << m_future->symbol << " " << m_order << std::endl;
  }
}

optional<string> gamma_scalper::update_deltas(double time_to_expiration) {
  // Underlying price
  auto underlying_price = details::get_price(m_future->bbo);
  if (!underlying_price) {
    // Impossible to do calculations if there's no underlying price
    return optional<string>("Missing underlying price");
  }

  double cost_of_carry = m_interest_rate;

  // Calculating mid prices. If any is missing, try to get the other using the
  // put-call parity property of european options
  auto call_price = details::get_call_price(
      m_straddle_call->bbo, m_straddle_put->bbo, underlying_price,
      time_to_expiration, *m_straddle_call->strike_price, m_interest_rate);
  auto put_price = details::get_put_price(
      m_straddle_call->bbo, m_straddle_put->bbo, underlying_price,
      time_to_expiration, *m_straddle_call->strike_price, m_interest_rate);

  // If after everything we have no prices. Ignore this cicle... imposible to
  // calculate deltas without market
  if (!call_price || !put_price) {
    return optional<string>("Missing prices");
  }
  // If there's one below 0, correct it to 0. This could happen after the
  // put-call parity price estimation
  call_price = *call_price < 0 ? optional<price_t>(0) : call_price;
  put_price = *put_price < 0 ? optional<price_t>(0) : put_price;

  // calculating call delta
  auto call_delta = details::get_delta(
      option_type_t::CALL, *underlying_price, *m_straddle_call->strike_price,
      m_interest_rate, time_to_expiration, cost_of_carry, *call_price);

  // calculating put delta
  auto put_delta = details::get_delta(
      option_type_t::PUT, *underlying_price, *m_straddle_call->strike_price,
      m_interest_rate, time_to_expiration, cost_of_carry, *put_price);
  //  put deltas are supposed to be negative. Reverting sign if there's a value
  if (put_delta) {
    put_delta = -*put_delta;
  }

  // We have at leas one delta
  if (!call_delta && !put_delta) {
    return string("Missing both deltas");
  }

  // Trying to deduce one delta from the other (dS + dP = dC) in case one is
  // missing
  if (!call_delta && put_delta) {
    if (*put_delta > 0) {
      put_delta = 0.0;
    }
    call_delta = 1 + *put_delta;
  } else if (call_delta && !put_delta) {
    if (*call_delta < 0) {
      call_delta = 0.0;
    }
    put_delta = 1 - *call_delta;
  }

  // If any delta is nan, skip this cicle
  if (isnan(*call_delta) || isnan(*put_delta)) {
    return optional<string>("Some delta is NaN");
  }

  // Updating cached values taking into consideration the position
  auto &future_position = m_positions[m_future->symbol];
  int future_quantity_sign =
      future_position.position.side == side_t::BUY ? 1 : -1;
  m_delta_future = ((future_position.position.quantity * future_quantity_sign) *
                    *future_position.instrument.contract_multiplier) /
                   *underlying_price;

  auto &call_position = m_positions[m_straddle_call->symbol];
  int call_quantity_sign = call_position.position.side == side_t::BUY ? 1 : -1;
  m_delta_call = *call_delta *
                 (call_position.position.quantity * call_quantity_sign) *
                 *call_position.instrument.contract_multiplier;

  auto &put_position = m_positions[m_straddle_put->symbol];
  int put_quantity_sign = put_position.position.side == side_t::BUY ? 1 : -1;
  m_delta_put = *put_delta *
                (put_position.position.quantity * put_quantity_sign) *
                *put_position.instrument.contract_multiplier;

  std::cout << " Underlying price: " << underlying_price << std::endl;
  std::cout << " call price      : " << call_price << std::endl;
  std::cout << " put  price      : " << put_price << std::endl;

  return boost::none;
}

void gamma_scalper::report_error(string const &message) {
  print_report();
  std::cerr << message << std::endl;
  BOOST_THROW_EXCEPTION(std::runtime_error(message));
}

void gamma_scalper::cancel_all_orders() {
  if (!m_order) {
    // Nothing to cancel
    return;
  }

  // Cancel and remove
  m_market->send_mass_cancellation_order();
  m_order = optional<order_t>(boost::none);
}

void gamma_scalper::update_position(execution_report_t &report) {
  std::stringstream ss;
  ss << report;

  if ((m_positions.find(*report.symbol) == m_positions.end()) ||
      !report.average_execution_price || !report.side ||
      !report.executed_volume || !report.average_execution_price) {
    // If pre-conditions don't apply, don't update position
    return;
  }

  std::cout << "Updating position" << std::endl;
  // Updating position quantity and sign
  auto &position = m_positions[*report.symbol];
  auto const filled_volume = *report.side == side_t::BUY
                                 ? *report.executed_volume
                                 : -*report.executed_volume;
  auto const signed_quantity = position.position.side == side_t::BUY
                                   ? position.position.quantity
                                   : -position.position.quantity;
  auto const new_quantity = signed_quantity + filled_volume;

  std::cout << "filled_volume: " << filled_volume << std::endl;
  std::cout << "signed_quantity: " << signed_quantity << std::endl;
  std::cout << "new_quantity: " << new_quantity << std::endl;

  position.position.quantity = std::abs(new_quantity);
  position.position.side = new_quantity >= 0 ? side_t::BUY : side_t::SELL;

  // Updating prices
  position.position.settlement_price = *report.average_execution_price;
  position.position.underlying_end_price =
      static_cast<price_t>(*details::get_price(m_future->bbo));

  std::cout << "position: " << position.position << std::endl;

  // Updating levels
  m_levels.update_levels(*report.executed_volume,
                         *report.average_execution_price, *report.side,
                         *m_future);
}
