#include "config.h"

#include "gamma_scalper/gamma_scalper.h"
#include "testing_strategy.h"

#include "argparse/argparse.h"
#include "config_file/config_file.h"

#include <chrono>
#include <thread>

/**
 * Creates the entry for the program
 */
int main(int argc, const char **argv) {
  // Check for arguments
  ArgumentParser parser;
  parser.addArgument("-u", "--user_config", 1, false);
  parser.addArgument("-s", "--strategy", 1, true);
  parser.parse(argc, argv);

  // Read configuration
  auto user_configuration_file = parser.retrieve<std::string>("user_config");

  // Load the user configuration file
  auto configuration = config_file::load_config_file(user_configuration_file);
  if (configuration.empty()) {
    std::cerr << "ERROR: Impossible to process the configuration file"
              << std::endl;
    return 1;
  }

  if (parser.retrieve<std::string>("strategy") == "gamma_scalper") {
    auto strategy = std::make_unique<gamma_scalper>(configuration);
    while (strategy->run()) {
      strategy.reset();
      std::this_thread::sleep_for(std::chrono::minutes(5));
      strategy = std::make_unique<gamma_scalper>(configuration);
    }
  } else {
    testing_strategy strategy(configuration);
    while (strategy.run()) {
    }
  }

  return 0;
}
