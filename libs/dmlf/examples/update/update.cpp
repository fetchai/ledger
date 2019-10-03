//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "core/logging.hpp"
#include "dmlf/update.hpp"

namespace {

constexpr char const *LOGGING_NAME = "main";

using fetch::dmlf::UpdateInterface;
using fetch::dmlf::Update;

using namespace std::chrono_literals;

}  // namespace

int main(int argc, char **argv)
{
  FETCH_LOG_INFO(LOGGING_NAME, "MAIN ", argc, " ", argv);
  std::shared_ptr<UpdateInterface> udt_1 = std::make_shared<Update<int>>(std::vector<int>{1, 2, 4});
  FETCH_LOG_INFO(LOGGING_NAME, "Update1 ", udt_1->TimeStamp(), " ", udt_1->DebugString());
  std::this_thread::sleep_for(1s);
  std::shared_ptr<UpdateInterface> udt_2 = std::make_shared<Update<int>>(std::vector<int>{1, 2, 4});
  FETCH_LOG_INFO(LOGGING_NAME, "Update2 ", udt_2->TimeStamp(), " ", udt_2->DebugString());
  FETCH_LOG_INFO(LOGGING_NAME, "Update1 < Update2 ? ", udt_2 > udt_1);

  std::shared_ptr<UpdateInterface> udt_3 = std::make_shared<Update<int>>(std::vector<int>{1, 2, 5});
  FETCH_LOG_INFO(LOGGING_NAME, "Update3 ", udt_3->TimeStamp(), " ", udt_3->DebugString());
  std::this_thread::sleep_for(1.54321s);
  std::shared_ptr<UpdateInterface> udt_4 = std::make_shared<Update<int>>();
  FETCH_LOG_INFO(LOGGING_NAME, "Update4 ", udt_4->TimeStamp(), " ", udt_4->DebugString());
  auto udt_3_bytes = udt_3->Serialise();
  udt_4->DeSerialise(udt_3_bytes);
  FETCH_LOG_INFO(LOGGING_NAME, "Update4 ", udt_4->TimeStamp(), " ", udt_4->DebugString());

  return EXIT_SUCCESS;
}
