#include "levels.h"

#include <limits>
#include <regex>

namespace details {

string const levels_file = "levels";
string const pnl_file = "pnl";
string const pnl_log_file = "pnl_log";

}  // namespace details

levels::levels(string aux_folder_path, double price_sweetener)
    : m_aux_folder_path(aux_folder_path),
      m_levels(),
      m_price_sweetener(price_sweetener) {
  load_levels();
}

levels::~levels() { store_levels(); }

void levels::store_levels() {
  auto const file_path = m_aux_folder_path + details::levels_file;
  std::ofstream file;
  file.open(file_path, std::ios::out);

  for (auto const level : m_levels) {
    int side = level.side == side_t::BUY ? 1 : 2;
    file << level.price << ";" << side << ";" << level.volume << std::endl;
  }

  file.close();
}

void levels::load_levels() {
  auto const file_path = m_aux_folder_path + details::levels_file;
  std::ifstream file;
  file.open(file_path, std::ios::in);

  string line;
  while (std::getline(file, line)) {
    std::smatch cm;
    std::regex_match(line, cm, std::regex("(.*?);(.*?);(.*?)"));
    string match = cm.str(3);

    auto level_price = static_cast<price_t>(std::atof(cm.str(1).c_str()));
    auto side = static_cast<side_t>(std::atoi(cm.str(2).c_str()));
    auto level_volume = static_cast<volume_t>(std::atof(cm.str(3).c_str()));

    m_levels.push_back({level_volume, level_price, side});
  }
  file.close();
}

void levels::store_pnl(price_t front_price, price_t report_price,
                       side_t report_side, volume_t raw_paired_volume,
                       instrument_t const& future) {
  auto const file_path = m_aux_folder_path + details::pnl_file;
  auto const log_file_path = m_aux_folder_path + details::pnl_log_file;

  double calculated_pnl;
  auto const paired_volume = raw_paired_volume * *future.contract_multiplier;

  auto top_value = paired_volume / front_price;
  top_value = report_side == side_t::SELL ? top_value : -top_value;

  auto report_value = paired_volume / report_price;
  report_value = report_side == side_t::SELL ? -report_value : report_value;

  calculated_pnl = report_value + top_value;

  // Read PNL from file
  std::ifstream input_file;
  input_file.open(file_path, std::ios::in);

  string line;
  optional<double> pnl = optional<double>(boost::none);
  std::getline(input_file, line);
  pnl = atof(line.c_str());

  input_file.close();

  // Store PNL in the file
  std::ofstream output_file;
  output_file.open(file_path, std::ios::out);

  auto to_write = pnl ? *pnl + calculated_pnl : calculated_pnl;
  output_file << to_write << std::endl;

  output_file.close();

  // Store PNL log in the file
  std::ofstream output_log_file;
  output_log_file.open(log_file_path, std::ios::app);

  output_log_file << "Formula: " << std::endl;
  output_log_file << "top_value = " << paired_volume << " / " << front_price
                  << " = " << top_value << std::endl;
  output_log_file << "report_value = " << paired_volume << " / " << report_price
                  << " = " << report_value << std::endl;
  output_log_file << "report side : " << to_string(report_side) << std::endl;
  output_log_file << top_value << " + " << report_value << " = "
                  << calculated_pnl << std::endl;

  output_log_file.close();
}

void levels::update_levels(volume_t traded_volume, price_t traded_price,
                           side_t side, instrument_t const& future) {
  // Update PnL
  // If we have no levels or front is the same side, insert
  if (m_levels.empty() || (m_levels.front().side == side)) {
    m_levels.push_front({traded_volume, traded_price, side});
  } else {
    // Substract current execution from level
    auto& front = m_levels.front();
    auto const front_volume = front.volume;
    auto const front_price = front.price;
    auto filled_volume =
        front_volume < traded_volume ? front_volume : traded_volume;

    front.volume -= traded_volume;

    if (front.volume == 0) {
      // If reminder volume is == 0 remove that level
      m_levels.pop_front();
    } else if (front.volume < 0) {
      // If < 0, remove this level and process next level
      m_levels.pop_front();
      traded_volume -= front_volume;
      update_levels(traded_volume, traded_price, side, future);
    } else {
      // Executed volume was not enough to cover the whole level
      front.volume = traded_volume;
    }
    store_pnl(front_price, traded_price, side, filled_volume, future);
  }

  store_levels();
  std::cout << "m_levels.size(): " << m_levels.size() << std::endl;
}

price_t levels::get_price_to_use(side_t const& side,
                                 instrument_t const& future) {
  // If front from levels is the same side use future price
  if (m_levels.empty()) {
    return side == side_t::BUY ? (*future.bbo->bid) : (*future.bbo->ask);
  }

  price_t to_return(0);
  if (side == side_t::BUY) {
    auto front_price =
        static_cast<price_t>(m_levels.front().price -
                             (*future.contract_multiplier * m_price_sweetener));
    to_return = *future.bbo->bid < front_price ? *future.bbo->bid : front_price;
  } else {
    auto front_price =
        static_cast<price_t>(m_levels.front().price +
                             (*future.contract_multiplier * m_price_sweetener));
    to_return = *future.bbo->ask > front_price ? *future.bbo->ask : front_price;
  }
  return to_return;
}

volume_t levels::get_volume_to_use(side_t const& side,
                                   volume_t corrections_todo) {
  // If front from levels is the same side use correction_todo
  if (m_levels.empty() || (side == m_levels.front().side)) {
    return volume_t(corrections_todo);
  }

  // Return smallest
  auto const to_return = corrections_todo < m_levels.front().volume
                             ? corrections_todo
                             : m_levels.front().volume;
  return volume_t(to_return);
}
