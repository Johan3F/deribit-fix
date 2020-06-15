#pragma once

#include <ostream>
#include "basic_types.h"

struct position_t {
  string symbol;
  volume_t quantity;
  side_t side;
  price_t settlement_price;
  price_t underlying_end_price;

  friend std::ostream &operator<<(std::ostream &os, position_t const &pos) {
    os << "Position [" << pos.symbol << "]-> #" << pos.quantity << " "
       << pos.settlement_price << " " << to_string(pos.side)
       << " Underlying price=" << pos.underlying_end_price;
    return os;
  }
};

using positions_list_t = vector<position_t>;
