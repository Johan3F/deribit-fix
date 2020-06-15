#pragma once

#include "basic_types.h"

// Generic case
template <typename T>
inline string to_string(T const& object) {
  return std::to_string(object);
}

// Trivial case for when it's an optional for string
template <>
inline string to_string(string const& object) {
  return object;
}

// Specializations
// boolean
template <>
inline string to_string(bool const& object) {
  return object ? "True" : "False";
}

// side_t
template <>
inline string to_string(side_t const& object) {
  switch (object) {
    case side_t::BUY:
      return "BUY";
      break;
    case side_t::SELL:
      return "SELL";
      break;
  }
  BOOST_THROW_EXCEPTION(
      std::runtime_error("to_string: Enumeration side_t has a wrong value"));
}

// option_type_t
template <>
inline string to_string(option_type_t const& object) {
  switch (object) {
    case option_type_t::CALL:
      return "CALL";
      break;
    case option_type_t::PUT:
      return "PUT";
      break;
  }
  BOOST_THROW_EXCEPTION(std::runtime_error(
      "to_string: Enumeration option_type_t has a wrong value"));
}

// price
template <>
inline string to_string(price_t const& object) {
  return to_string(object.t);
}

// volume
template <>
inline string to_string(volume_t const& object) {
  return to_string(object.t);
}

// currency
template <>
inline string to_string(currency const& object) {
  return object.t;
}

// ptime
template <>
inline string to_string(ptime const& object) {
  return to_iso_string(object);
}

// order_status_t
template <>
inline string to_string(order_status_t const& object) {
  switch (object) {
    case order_status_t::NEW:
      return "NEW";
      break;
    case order_status_t::PARTIAL:
      return "PARTIAL";
      break;
    case order_status_t::FILLED:
      return "FILLED";
      break;
    case order_status_t::CANCELED:
      return "CANCELED";
      break;
    case order_status_t::REJECTED:
      return "REJECTED";
      break;
  }
  BOOST_THROW_EXCEPTION(std::runtime_error(
      "to_string: Enumeration order_status_t has a wrong value"));
}

// order_type_t
template <>
inline string to_string(order_type_t const& object) {
  switch (object) {
    case order_type_t::MARKET:
      return "MARKET";
      break;
    case order_type_t::LIMIT:
      return "LIMIT";
      break;
  }
  BOOST_THROW_EXCEPTION(std::runtime_error(
      "to_string: Enumeration order_type_t has a wrong value"));
}

// volume_type_t
template <>
inline string to_string(volume_type_t const& object) {
  switch (object) {
    case volume_type_t::CONTRACTS:
      return "CONTRACTS";
      break;
  }
  BOOST_THROW_EXCEPTION(std::runtime_error(
      "to_string: Enumeration volume_type_t has a wrong value"));
}

// mass_cancelation_type_t
template <>
inline string to_string(mass_cancelation_type_t const& object) {
  switch (object) {
    case mass_cancelation_type_t::ALL:
      return "All";
      break;
    case mass_cancelation_type_t::BY_SECURITY_TYPE:
      return "By security type";
      break;
    case mass_cancelation_type_t::BY_SYMBOL:
      return "By symbol";
      break;
  }
  BOOST_THROW_EXCEPTION(std::runtime_error(
      "to_string: Enumeration mass_cancelation_type_t has a wrong value"));
}

// mass_cancelation_error_t
template <>
inline string to_string(mass_cancelation_error_t const& object) {
  switch (object) {
    case mass_cancelation_error_t::UNKNOWN_SECURITY:
      return "Unknown security";
      break;
    case mass_cancelation_error_t::UNKNOWN_SECURITY_TYPE:
      return "Unknown security type";
      break;
  }
  BOOST_THROW_EXCEPTION(std::runtime_error(
      "to_string: Enumeration mass_cancelation_error_t has a wrong value"));
}

// Optionals
template <typename T>
inline string to_string(optional<T> const& object) {
  if (object) {
    return to_string(*object);
  }
  return "Empty";
}
