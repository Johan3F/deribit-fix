#pragma once

#include "../config.h"

struct order_t {
  string id;
  string original_id;
  side_t side;
  price_t order_price;
  volume_t full_volume;
  volume_t open_volume;

  friend std::ostream &operator<<(std::ostream &os, order_t const &order) {
    os << to_string(order.side) << " -> ";
    os << order.id << " [" << order.original_id << "] ";
    os << to_string(order.order_price) << " #" << to_string(order.full_volume);
    os << " [" << to_string(order.open_volume) << "]";
    return os;
  }
};
