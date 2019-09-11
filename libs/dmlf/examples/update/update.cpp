#include <iostream>
#include <cstdlib>
#include <memory>
#include <thread>
#include <chrono>

#include "dmlf/update.hpp"
#include "core/logging.hpp"

namespace {
constexpr char const *LOGGING_NAME = "main";

using fetch::dmlf::IUpdate;
using fetch::dmlf::Update;

using namespace std::chrono_literals;
}  // namespace

int main(int argc, char **argv)
{
  FETCH_LOG_INFO(LOGGING_NAME, "MAIN ", argc, " ", argv);
  std::shared_ptr<IUpdate> udt_1 = std::make_shared<Update<std::string>>();
  FETCH_LOG_INFO(LOGGING_NAME, "Update1 ", udt_1->TimeStamp());
  std::this_thread::sleep_for(1s);
  std::shared_ptr<IUpdate> udt_2 = std::make_shared<Update<std::string>>();
  FETCH_LOG_INFO(LOGGING_NAME, "Update2 ", udt_2->TimeStamp());
  FETCH_LOG_INFO(LOGGING_NAME, "Update1 < Update2 ? ", udt_2 > udt_1);

  return EXIT_SUCCESS;
}


