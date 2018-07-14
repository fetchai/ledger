#include "core/commandline/cli_header.hpp"
#include "core/commandline/parameter_parser.hpp"
#include "core/commandline/params.hpp"
#include "network/management/network_manager.hpp"

#include "constellation.hpp"

#include <iostream>

//namespace {
//
//struct CommandLineArguments {
//  std::size_t lanes{1};
//
//  static CommandLineArguments Parse(int argc, char **argv) {
//    CommandLineArguments args;
//
//    fetch::commandline::Params parameters;
//    parameters.add(args.lanes, "lanes", "The number of lanes to be configured");
//
//    parameters.Parse(argc, const_cast<char const **>(argv));
//
//    return args;
//  }
//
//  friend std::ostream &operator<<(std::ostream &s, CommandLineArguments const &args) {
//    s << "lanes..: " << args.lanes << '\n';
//    return s;
//  }
//};
//
//} // namespace

int main(int argc, char **argv) {
  int exit_code = EXIT_FAILURE;

  fetch::commandline::DisplayCLIHeader("Constellation");

  try {

    // create and run the constellation
    auto constellation = fetch::Constellation::Create(1, 2);
    constellation->Run();

    exit_code = EXIT_SUCCESS;
  } catch (std::exception &ex) {
    std::cerr << "Fatal Error: " << ex.what() << std::endl;
  }

  return exit_code;
}