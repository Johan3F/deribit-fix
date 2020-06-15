#pragma once

#include "../definitions/basic_types.h"

/**
 * Calculates price of an option using the black-scholes-merton general formula
 */
double generalized_black_scholes_merton(option_type_t call_or_put,
                                        double stock_price, double strike_price,
                                        double risk_free_interest,
                                        double time_to_expiration,
                                        double cost_of_carry,
                                        double volatility);

/**
 * Extracts the implied volatility of an option via black-scholes
 */
optional<double> black_scholes_implied_volatility(
    option_type_t call_or_put, double stock_price, double strike_price,
    double time_to_expiration, double risk_free_interest, double cost_of_carry,
    double option_market_price);

/**
 * Extracts the delta of an option via black-scholes
 */
double black_scholes_delta(option_type_t call_or_put, double stock_price,
                           double strike_price, double risk_free_interest,
                           double time_to_expiration, double cost_of_carry,
                           double volatility);
