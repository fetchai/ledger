//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include <iomanip>

namespace fetch {
namespace dmlf {

ExecutionResult testEtchExec(const char *src, const std::string &name, variant::Variant const &x)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("helloWorld", {{"etch", src}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("helloWorld", "state", name, {x});
  EXPECT_TRUE(result.succeeded()) << result.error().message() << std::endl;
  return result;
}

TEST(DISABLED_VarConverterTest, callWabble)
{

  static char const *TEXT4 = R"(
    function main(x : Array<Int32>) : Int32
      return
         x[0]+x[1]+x[2]+x[3]
         ;
    endfunction
  )";

  auto arr4 = fetch::variant::Variant::Array(4);
  for (std::size_t i = 0; i < 4; i++)
  {
    arr4[i] = i + 1;
  }

  std::cout << "TEST4..." << std::endl;
  auto foo4 = testEtchExec(TEXT4, "main", arr4);
  EXPECT_TRUE(foo4.output().IsInteger());
  EXPECT_EQ(foo4.output().As<uint32_t>(), 10);

  static char const *TEXT44 = R"(
    function main(x : Array<Array<Int32>>) : Int32
      return
         x[0][0]+x[1][0]+x[2][0]+x[3][0]+
         x[0][1]+x[1][1]+x[2][1]+x[3][1]+
         x[0][2]+x[1][2]+x[2][2]+x[3][2]+
         x[0][3]+x[1][3]+x[2][3]+x[3][3]
      ;
    endfunction
  )";

  auto arr44 = fetch::variant::Variant::Array(4);
  arr44[0]   = fetch::variant::Variant::Array(4);
  arr44[1]   = fetch::variant::Variant::Array(4);
  arr44[2]   = fetch::variant::Variant::Array(4);
  arr44[3]   = fetch::variant::Variant::Array(4);

  for (std::size_t i = 0; i < 4; i++)
  {
    for (std::size_t j = 0; j < 4; j++)
    {
      arr44[i][j] = i * j;
    }
  }

  std::cout << "TEST44..." << std::endl;
  auto foo44 = testEtchExec(TEXT44, "main", arr44);
  EXPECT_TRUE(foo44.output().IsInteger());
  EXPECT_EQ(foo44.output().As<uint32_t>(), 10);
}

}  // namespace dmlf
}  // namespace fetch
