#include "core/make_unique.hpp"
#include "core/commandline/params.hpp"
#include "network/p2pservice/p2p_service.hpp"

#include <iostream>
#include <cstdlib>
#include <memory>
#include <atomic>
#include <chrono>
#include <thread>

namespace {

  constexpr std::size_t DEFAULT_NUM_THREADS = 4;
  constexpr uint16_t DEFAULT_PORT = 8001;

  using underlying_p2p_service_type = fetch::p2p::P2PService;
  using p2p_service_type = std::unique_ptr<underlying_p2p_service_type>;
  using running_flag_type = std::atomic<bool>;

  running_flag_type global_running_flag_{true};

  struct CommandlineArgs {
    uint16_t port{0};

    static CommandlineArgs Parse(int argc, char **argv) {
      CommandlineArgs args;

      fetch::commandline::Params parser;
      parser.add(args.port, "port", "The port to run the P2P service from", DEFAULT_PORT);

      parser.Parse(argc, const_cast<char const **>(argv));

      return args;
    }
  };

  void run(int argc, char **argv) {

    // parse the command line
    auto args = CommandlineArgs::Parse(argc, argv);

    fetch::logger.Info("Running Nebula bootstrap server on rpc://0.0.0.0:", args.port);

    fetch::network::NetworkManager network_manager{DEFAULT_NUM_THREADS};

    // create the P2P service
    p2p_service_type service = fetch::make_unique<underlying_p2p_service_type>(args.port, network_manager);

    network_manager.Start();
    service->Start();

    while (global_running_flag_) {
      std::this_thread::sleep_for(std::chrono::seconds{1});
    }

    service->Stop();
    network_manager.Stop();
  }

} // namespace

int main(int argc, char **argv) {

  try {
    run(argc, argv);
  } catch (std::exception &ex) {
    std::cerr << "Fatal Error: " << ex.what() << std::endl;
  } catch (...) {
    std::cerr << "Fatal Error: Internal Error" << std::endl;
  }

  return 0;
}
