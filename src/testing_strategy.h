#pragma once

#include "config.h"

#include "quickfix/quickfix.h"

class testing_strategy : public FIX::quickfix_user {
 public:
  /** Constructor */
  testing_strategy(config_file_t &configuration);

  /** Destructor */
  ~testing_strategy();

  /** Run the strategy */
  bool run();

 private:
  // Configuration file
  config_file_t &m_configuration;

  std::unique_ptr<FIX::quickfix> m_market;
};
