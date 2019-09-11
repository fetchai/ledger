#include <iostream>
#include <cstdlib>
#include <memory>
#include <thread>
#include <chrono>
#include <vector>

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
  std::shared_ptr<IUpdate> udt_1 = std::make_shared<Update<int>>(std::vector<int>{1,2,4});
  FETCH_LOG_INFO(LOGGING_NAME, "Update1 ", udt_1->TimeStamp(), " ", udt_1->debug());
  std::this_thread::sleep_for(1s);
  std::shared_ptr<IUpdate> udt_2 = std::make_shared<Update<int>>(std::vector<int>{1,2,4});
  FETCH_LOG_INFO(LOGGING_NAME, "Update2 ", udt_2->TimeStamp(), " ", udt_2->debug());
  FETCH_LOG_INFO(LOGGING_NAME, "Update1 < Update2 ? ", udt_2 > udt_1);
  
  std::shared_ptr<IUpdate> udt_3 = std::make_shared<Update<int>>(std::vector<int>{1,2,5});
  FETCH_LOG_INFO(LOGGING_NAME, "Update3 ", udt_3->TimeStamp(), " ", udt_3->debug());
  std::this_thread::sleep_for(1.54321s);
  std::shared_ptr<IUpdate> udt_4 = std::make_shared<Update<int>>();
  FETCH_LOG_INFO(LOGGING_NAME, "Update4 ", udt_4->TimeStamp(), " ", udt_4->debug());
  auto udt_3_bytes = udt_3->serialise();
  udt_4->deserialise(udt_3_bytes);
  FETCH_LOG_INFO(LOGGING_NAME, "Update4 ", udt_4->TimeStamp(), " ", udt_4->debug());

  return EXIT_SUCCESS;
}


