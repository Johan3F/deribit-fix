#pragma once

#include "basic_types.h"
#include "to_string.h"

struct execution_report_t {
  optional<string> order_id;
  optional<string> original_order_id;
  optional<order_status_t> order_status;
  optional<side_t> side;
  optional<ptime> transaction_time;
  optional<volume_t> open_volume;
  optional<volume_t> executed_volume;
  optional<volume_t> order_volume;
  optional<order_type_t> order_type;
  optional<int> reject_reason;
  optional<string> symbol;

  optional<price_t> order_price;
  optional<volume_type_t> volume_type;
  optional<double> contract_multiplier;
  optional<price_t> average_execution_price;
  optional<volume_t> maximun_show_volume;
  optional<double> implied_volatility;
  optional<price_t> pegged_price;

  optional<int> mass_status_request_type;
  optional<int> mass_status_report_number;

  friend std::ostream &operator<<(std::ostream &os,
                                  execution_report_t const &report) {
    auto const &separation_string = "|";
    os << "Execution report [" << to_string(report.order_id) << " - "
       << to_string(report.original_order_id)
       << "]: " << to_string(report.symbol) << separation_string;

    os << "Status: " << to_string(report.order_status) << separation_string;
    os << "Side: " << to_string(report.side) << separation_string;
    os << "Transation time: " << to_string(report.transaction_time)
       << separation_string;
    os << "Open volume: " << to_string(report.open_volume) << separation_string;
    os << "Executed volume: " << to_string(report.executed_volume)
       << separation_string;
    os << "Order volume: " << to_string(report.order_volume)
       << separation_string;
    os << "Order type: " << to_string(report.order_type) << separation_string;
    if (report.order_type == order_type_t::LIMIT) {
      os << "Order price: " << to_string(report.order_price)
         << separation_string;
    }
    os << "Volume type: " << to_string(report.volume_type) << separation_string;
    os << "Contract multiplier: " << to_string(report.contract_multiplier)
       << separation_string;
    os << "Execution price: " << to_string(report.average_execution_price)
       << separation_string;
    os << "Max show volume: " << to_string(report.maximun_show_volume)
       << separation_string;
    os << "Implied volatility: " << to_string(report.implied_volatility)
       << separation_string;
    os << "Pegged price: " << to_string(report.pegged_price)
       << separation_string;
    os << "Mass status reports comming: "
       << to_string(report.mass_status_report_number) << separation_string;

    return os;
  }
};
