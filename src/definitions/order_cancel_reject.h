#pragma once

#include "../config.h"
#include "to_string.h"

struct order_cancel_reject_t {
  string order_id;
  string original_order_id;

  optional<order_status_t> order_status;
  optional<string> reason;

  friend std::ostream &operator<<(std::ostream &os,
                                  order_cancel_reject_t const &mes) {
    os << "Order cancelation rejection: ";
    os << mes.order_id << " [" << mes.original_order_id << "] -> ";
    os << to_string(mes.order_status) << ": " << to_string(mes.reason);
    return os;
  }
};
