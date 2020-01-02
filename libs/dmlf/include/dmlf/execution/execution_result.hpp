#pragma once
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

#include "core/serializers/base_types.hpp"
#include "core/serializers/main_serializer.hpp"
#include "dmlf/execution/execution_error_message.hpp"
#include "network/generics/promise_of.hpp"
#include "variant/variant.hpp"

namespace fetch {
namespace dmlf {

class ExecutionResult
{
public:
  using Variant         = fetch::variant::Variant;
  using Error           = ExecutionErrorMessage;
  using ErrorCode       = ExecutionErrorMessage::Code;
  using ErrorStage      = ExecutionErrorMessage::Stage;
  using PromiseOfResult = fetch::network::PromiseOf<ExecutionResult>;

  ExecutionResult() = default;

  ExecutionResult(Variant output, Error error, std::string console)
    : output_(std::move(output))
    , error_(std::move(error))
    , console_(std::move(console))
  {}

  static ExecutionResult MakeSuccess();
  static ExecutionResult MakeIntegerResult(int r);

  Variant output() const
  {
    return output_;
  }
  Error error() const
  {
    return error_;
  }
  std::string console() const
  {
    return console_;
  }

  bool succeeded() const
  {
    return error_.code() == ErrorCode::SUCCESS;
  }

  static ExecutionResult MakeResultFromStatus(Error const &status);
  static ExecutionResult MakeSuccessfulResult();
  static ExecutionResult MakeErroneousResult(ErrorCode err_code, std::string const &err_msg);

  static PromiseOfResult MakePromise();
  static void            FulfillPromise(PromiseOfResult &promise, ExecutionResult const &fulfiller);
  static PromiseOfResult MakeFulfilledPromise(ExecutionResult const &fulfiller);
  static PromiseOfResult MakeFulfilledPromise(Error &error);
  static PromiseOfResult MakeFulfilledPromiseSuccess();
  static PromiseOfResult MakeFulfilledPromiseError(ErrorCode          error_code,
                                                   std::string const &error_message);

private:
  Variant     output_;
  Error       error_;
  std::string console_;

  template <typename T, typename D>
  friend struct serializers::MapSerializer;
};

}  // namespace dmlf

namespace serializers {

template <typename D>
struct MapSerializer<fetch::dmlf::ExecutionResult, D>
{
public:
  using Type       = fetch::dmlf::ExecutionResult;
  using DriverType = D;

  static uint8_t const OUTPUT  = 1;
  static uint8_t const ERROR   = 2;
  static uint8_t const CONSOLE = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &exec_res)
  {
    auto             map    = map_constructor(3);
    variant::Variant output = exec_res.output_;

    if (output.IsUndefined())
    {
      // MsgPack does not support undefined
      // unless extended.
      output = static_cast<std::string>("");
    }

    map.Append(OUTPUT, output);
    map.Append(ERROR, exec_res.error_);
    map.Append(CONSOLE, exec_res.console_);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &exec_res)
  {
    map.ExpectKeyGetValue(OUTPUT, exec_res.output_);
    map.ExpectKeyGetValue(ERROR, exec_res.error_);
    map.ExpectKeyGetValue(CONSOLE, exec_res.console_);
  }
};

}  // namespace serializers
}  // namespace fetch
