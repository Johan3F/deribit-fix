#include "black_scholes.h"

#include <complex>

namespace details {

double cummulative_normal_distribution_hart(double x) {
  double cnd = 0.0;

  double y = std::abs(x);

  double const a1 = 0.0352624965998911;
  double const a2 = 0.700383064443688;
  double const a3 = 6.37396220353165;
  double const a4 = 33.912866078383;
  double const a5 = 112.079291497871;
  double const a6 = 221.213596169931;
  double const a7 = 220.206867912376;

  double const b1 = 0.0883883476483184;
  double const b2 = 1.75566716318264;
  double const b3 = 16.064177579207;
  double const b4 = 86.7807322029461;
  double const b5 = 296.564248779674;
  double const b6 = 637.333633378831;
  double const b7 = 793.826512519948;
  double const b8 = 440.413735824752;

  if (y > 37) {
    return cnd;
  }

  double exponential = std::exp(-(y * y) / double(2.0));

  if (y < 7.07106781186547) {
    // clang-format off
			double sum_a = ((((((a1 * y + a2) * y + a3) * y + a4) * y + a5) * y + a6) * y + a7);

			double sum_b = (((((((b1 * y + b2) * y + b3) * y + b4) * y + b5) * y + b6) * y + b7)*y + b8);
    // clang-format on

    cnd = exponential * (sum_a / sum_b);
  } else {
    double sum_a = y + 1.0 / (y + 2.0 / (y + 3.0 / (y + 4.0 / (y + 0.65))));

    cnd = exponential / (sum_a * 2.506628274631);
  }

  if (x > 0) {
    cnd = 1.0 - cnd;
  }

  return cnd;
}

double black_scholes_d1(double stock_price, double strike_price,
                        double time_to_expiration, double cost_of_carry,
                        double volatility) {
  return (std::log(stock_price / strike_price) +
          ((cost_of_carry + ((volatility * volatility) / 2)) *
           time_to_expiration)) /
         (volatility * std::sqrt(time_to_expiration));
}

double black_scholes_d2(double time_to_expiration, double volatility,
                        double d1) {
  return d1 - (volatility * std::sqrt(time_to_expiration));
}

}  // namespace details

double generalized_black_scholes_merton(option_type_t call_or_put,
                                        double stock_price, double strike_price,
                                        double risk_free_interest,
                                        double time_to_expiration,
                                        double cost_of_carry,
                                        double volatility) {
  double black_scholes_value = 0.0;

  // clang-format off
	double d1 = details::black_scholes_d1(stock_price, strike_price, time_to_expiration, cost_of_carry, volatility);
	double d2 = details::black_scholes_d2(time_to_expiration, volatility, d1);
  // clang-format on

  if (option_type_t::CALL == call_or_put) {
    // clang-format off
		black_scholes_value =
			(stock_price * std::exp((cost_of_carry - risk_free_interest) * time_to_expiration) * details::cummulative_normal_distribution_hart(d1)) -
			(strike_price * std::exp((-risk_free_interest) * time_to_expiration) * details::cummulative_normal_distribution_hart(d2));
    // clang-format on
  } else if (option_type_t::PUT == call_or_put) {
    // clang-format off
		black_scholes_value =
			(strike_price * std::exp((-risk_free_interest) * time_to_expiration) * details::cummulative_normal_distribution_hart(-d2)) -
			(stock_price * std::exp((cost_of_carry - risk_free_interest) * time_to_expiration) * details::cummulative_normal_distribution_hart(-d1));
    // clang-format on
  } else {
    throw(std::runtime_error("Option type for black-scholes not supported"));
  }

  return black_scholes_value;
}

optional<double> black_scholes_implied_volatility(
    option_type_t call_or_put, double stock_price, double strike_price,
    double time_to_expiration, double risk_free_interest, double cost_of_carry,
    double option_market_price) {
  int const MAX_ITERATIONS = 100;

  double volatility_low = 0.05;
  double volatility_high = 5;
  double epsilon = 0.000008;

  double option_price_low = generalized_black_scholes_merton(
      call_or_put, stock_price, strike_price, risk_free_interest,
      time_to_expiration, cost_of_carry, volatility_low);
  double option_price_high = generalized_black_scholes_merton(
      call_or_put, stock_price, strike_price, risk_free_interest,
      time_to_expiration, cost_of_carry, volatility_high);

  int counter = 0;

  double volatility =
      volatility_low + (option_market_price - option_price_low) *
                           (volatility_high - volatility_low) /
                           (option_price_high - option_price_low);
  double market_price_at_volatility = generalized_black_scholes_merton(
      call_or_put, stock_price, strike_price, risk_free_interest,
      time_to_expiration, cost_of_carry, volatility);
  while (std::abs(option_market_price - market_price_at_volatility) > epsilon) {
    ++counter;
    if (counter == MAX_ITERATIONS) {
      return boost::none;
    }

    if (market_price_at_volatility < option_market_price) {
      volatility_low = volatility;
      option_price_low = generalized_black_scholes_merton(
          call_or_put, stock_price, strike_price, risk_free_interest,
          time_to_expiration, cost_of_carry, volatility_low);
    } else {
      volatility_high = volatility;
      option_price_high = generalized_black_scholes_merton(
          call_or_put, stock_price, strike_price, risk_free_interest,
          time_to_expiration, cost_of_carry, volatility_high);
    }

    volatility = volatility_low + (option_market_price - option_price_low) *
                                      ((volatility_high - volatility_low) /
                                       (option_price_high - option_price_low));
    market_price_at_volatility = generalized_black_scholes_merton(
        call_or_put, stock_price, strike_price, risk_free_interest,
        time_to_expiration, cost_of_carry, volatility);
  }

  return volatility;
}

double black_scholes_delta(option_type_t call_or_put, double stock_price,
                           double strike_price, double risk_free_interest,
                           double time_to_expiration, double cost_of_carry,
                           double volatility) {
  double d1 = details::black_scholes_d1(
      stock_price, strike_price, time_to_expiration, cost_of_carry, volatility);
  if (option_type_t::PUT == call_or_put) {
    d1 = -d1;
  }
  return std::exp((cost_of_carry - risk_free_interest) * time_to_expiration) *
         details::cummulative_normal_distribution_hart(d1);
}
