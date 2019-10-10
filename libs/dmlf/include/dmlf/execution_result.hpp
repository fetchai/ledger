#pragma once
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

#include "core/serializers/base_types.hpp"
#include "core/serializers/main_serializer.hpp"
#include "dmlf/execution_error_message.hpp"
#include "vm/variant.hpp"

namespace fetch {
namespace dmlf {

class ExecutionResult
{
  template <typename T, typename D>
  friend struct serializers::MapSerializer;

public:
  using Variant = fetch::vm::Variant;
  using Error   = ExecutionErrorMessage;

  ExecutionResult() = default;

  ExecutionResult(Variant output, Error error, std::string console)
    : output_(output)
    , error_(error)
    , console_(std::move(console))
  {}

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

private:
  Variant     output_;
  Error       error_;
  std::string console_;
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
    auto map = map_constructor(3);
    map.Append(OUTPUT, exec_res.output_);
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
