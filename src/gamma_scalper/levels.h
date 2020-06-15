#pragma once

#include "../config.h"

/**
 * This structure will be stored in a stack to store information about the
 * buys and sells to avoid buying or selling above or below the last sold or
 * bought price
 */
struct level_t {
  volume_t volume;
  price_t price;
  side_t side;
};

class levels {
  using levels_t = std::deque<level_t>;

 public:
  // Constructor and destructor
  levels(string aux_folder_path, double m_price_sweetener);
  ~levels();

  /** Update the levels with a given execution report */
  void update_levels(volume_t filled, price_t traded_price, side_t side,
                     instrument_t const& future);

  /** @returns price that should be used based on levels */
  price_t get_price_to_use(side_t const& side, instrument_t const& future);

  /** @returns volume that should be used based on levels */
  volume_t get_volume_to_use(side_t const& side, volume_t corrections_todo);

 private:
  // Load and stores the levels from/into a file
  void store_levels();
  void load_levels();
  // Stores PNL in the PNL file
  void store_pnl(price_t front_price, price_t report_price, side_t report_side,
                 volume_t raw_paired_volume, instrument_t const& future);

  // Path to the auxiliar folder to store levels
  string m_aux_folder_path;

  // Storing information about the last buys and sells
  levels_t m_levels;

  // Sweetener to add to the price when calculating price for order
  double m_price_sweetener;
};
