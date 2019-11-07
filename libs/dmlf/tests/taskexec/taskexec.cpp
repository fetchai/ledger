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

#include "core/byte_array/byte_array.hpp"
#include "core/serializers/array_interface.hpp"
#include "core/serializers/container_constructor_interface.hpp"
#include "core/serializers/main_serializer.hpp"
#include "core/service_ids.hpp"
#include "dmlf/execution/basic_vm_engine.hpp"
#include "dmlf/execution/execution_engine_interface.hpp"
#include "dmlf/execution/execution_error_message.hpp"
#include "dmlf/execution/execution_params.hpp"
#include "dmlf/execution/execution_result.hpp"
#include "dmlf/remote_execution_client.hpp"
#include "dmlf/remote_execution_host.hpp"
#include "dmlf/remote_execution_protocol.hpp"
#include "dmlf/var_converter.hpp"
#include "gtest/gtest.h"
#include "oef-base/threading/Threadpool.hpp"

#include "oef-base/threading/Taskpool.hpp"
#include "dmlf/colearn/muddle_learner_networker_impl.hpp"

#include <iomanip>

namespace fetch {
namespace dmlf {

std::atomic<int> fooper(0);

class Fooper
  : public Task
{
public:
  virtual bool      IsRunnable() const
  {
    return true;
  }

  virtual ExitState run()
  {
    fooper++;
    if (fooper<5)
    {
      return ExitState::RERUN;
    }
    else
    {
      return ExitState::COMPLETE;
    }
  }
};

TEST(TaskExecTest, tasks_run)
{
  colearn::MuddleLearnerNetworkerImpl mh;

  auto fooping = std::make_shared<Fooper>();
  mh.submit(fooping);
  usleep(20000);

  ASSERT_TRUE(fooper >= 5);
}

}  // namespace dmlf
}  // namespace fetch
