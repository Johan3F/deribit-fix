#pragma once
#include "../config.h"

#include <quickfix/Application.h>

#include <type_traits>

namespace FIX {

template <typename ReturnType, typename = void>
struct field {
  template <typename MessateType>
  inline static ReturnType get(MessateType const& message, int const field_id);
};

// int
template <>
struct field<int> {
  template <typename MessateType>
  inline static int get(MessateType const& message, int const field_id) {
    return std::atoi(message.getField(field_id).c_str());
  }
};

// size_t
template <>
struct field<size_t> {
  template <typename MessateType>
  inline static size_t get(MessateType const& message, int const field_id) {
    return static_cast<size_t>(field<int>::get(message, field_id));
  }
};

// double
template <>
struct field<double> {
  template <typename MessateType>
  inline static double get(MessateType const& message, int const field_id) {
    return std::atof(message.getField(field_id).c_str());
  }
};

// price
template <>
struct field<price_t> {
  template <typename MessateType>
  inline static price_t get(MessateType const& message, int const field_id) {
    return static_cast<price_t>(field<double>::get(message, field_id));
  }
};

// volume
template <>
struct field<volume_t> {
  template <typename MessateType>
  inline static volume_t get(MessateType const& message, int const field_id) {
    return static_cast<volume_t>(field<double>::get(message, field_id));
  }
};

// string
template <>
struct field<string> {
  template <typename MessateType>
  inline static string get(MessateType const& message, int const field_id) {
    return message.getField(field_id);
  }
};

// currency
template <>
struct field<currency> {
  template <typename MessateType>
  inline static currency get(MessateType const& message, int const field_id) {
    return static_cast<currency>(field<string>::get(message, field_id));
  }
};

// ptime
template <>
struct field<ptime> {
  template <typename MessateType>
  inline static ptime get(MessateType const& message, int const field_id) {
    using time_facet = boost::posix_time::time_input_facet;
    string raw_date = field<string>::get(message, field_id);
    time_facet* facet = new time_facet("%Y%m%d-%H:%M:%s *");
    std::stringstream ss;
    ss.imbue(std::locale(std::locale(), facet));
    ss << raw_date;

    ptime pt;
    ss >> pt;

    return pt;
  }
};

// enumerations
template <typename T>
struct field<T, typename std::enable_if<std::is_enum<T>::value>::type> {
  template <typename MessateType>
  inline static T get(MessateType const& message, int const field_id) {
    return static_cast<T>(field<int>::get(message, field_id));
  }
};

// optional
template <typename T>
struct field<optional<T>> {
  template <typename MessateType>
  inline static optional<T> get(MessateType const& message,
                                int const field_id) {
    return message.isSetField(field_id) ? field<T>::get(message, field_id)
                                        : optional<T>(boost::none);
  }
};

}  // namespace FIX
