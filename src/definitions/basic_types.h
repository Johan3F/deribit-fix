#pragma once

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/optional.hpp>
#include <boost/optional/optional_io.hpp>
#include <boost/serialization/strong_typedef.hpp>

#include <string>
#include <vector>

// Not having to write std:: in many cases
using string = std::string;

template <typename T>
using vector = std::vector<T>;

// removing boost namespace
using namespace boost::posix_time;

template <typename T>
using optional = boost::optional<T>;

// Market primitives
BOOST_STRONG_TYPEDEF(double, price_t)
BOOST_STRONG_TYPEDEF(double, volume_t)
BOOST_STRONG_TYPEDEF(string, currency)

struct BBO_t {
  optional<volume_t> bid_volume;
  optional<price_t> bid;
  optional<price_t> ask;
  optional<volume_t> ask_volume;

  void clear() {
    bid_volume = boost::none;
    bid = boost::none;
    ask = boost::none;
    ask_volume = boost::none;
  }
};

// Enumerations
enum class side_t { BUY = 1, SELL = 2 };
enum class option_type_t { CALL = 1, PUT = 0 };
// TODO: Change the option_type_t enumeration back to production values
// Production is Call = 0 Put = 1. In test is the other way around
enum class order_status_t {
  NEW = 0,
  PARTIAL = 1,
  FILLED = 2,
  CANCELED = 4,
  REJECTED = 8
};
enum class order_type_t { MARKET = 1, LIMIT = 2 };
enum class volume_type_t { CONTRACTS = 1 };
enum class mass_cancelation_type_t {
  ALL = 1,
  BY_SECURITY_TYPE = 5,
  BY_SYMBOL = 7
};
enum class mass_cancelation_error_t {
  UNKNOWN_SECURITY = 1,
  UNKNOWN_SECURITY_TYPE = 5
};

// Operations with doubles
inline bool double_equals(double a, double b, double epsilon = 0.001) {
  return std::abs(a - b) < epsilon;
}
