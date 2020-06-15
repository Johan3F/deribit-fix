#pragma once

#include "../config.h"
#include "to_string.h"

struct mass_cancel_report_t {
  string order_id;
  mass_cancelation_type_t type;
  bool success;

  optional<mass_cancelation_error_t> error;

  friend std::ostream &operator<<(std::ostream &os,
                                  mass_cancel_report_t const &report) {
    os << "Mass cancelation report: ";
    os << report.order_id << " [" << to_string(report.type) << "]-> ";
    os << "success = " << to_string(report.success) << ". Error=";
    os << to_string(report.error);
    return os;
  }
};
