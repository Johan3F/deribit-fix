#pragma once

#include "basic_types.h"

struct instrument_t {
  string symbol;
  string description;
  string type;
  currency main_currency;
  optional<double> contract_multiplier;
  optional<option_type_t> put_call;
  optional<price_t> strike_price;
  optional<currency> strike_currency;
  optional<ptime> maturity_date;
  optional<volume_t> min_trade_volume;
  optional<double> tick_size;
  optional<BBO_t> bbo;

  friend std::ostream &operator<<(std::ostream &os, instrument_t const &instr) {
    os << "[" << to_string(instr.main_currency) << "] " << instr.symbol << " "
       << instr.description << " " << instr.type << " "
       << instr.contract_multiplier << " " << to_string(instr.min_trade_volume)
       << " " << to_string(instr.put_call) << " "
       << to_string(instr.strike_price) << " ["
       << to_string(instr.strike_currency) << "] "
       << to_string(instr.maturity_date);
    return os;
  }
};

using instruments_list_t = vector<instrument_t>;
