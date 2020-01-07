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

#include <string>

namespace fetch {
namespace dmlf {

class ExecutionErrorMessage
{
public:
  enum class Stage : uint32_t
  {
    ENGINE = 10,
    COMPILE,
    RUNNING,
    NETWORK,
  };

  enum class Code : uint32_t
  {
    SUCCESS = 0,

    BAD_TARGET = 100,
    BAD_EXECUTABLE,
    BAD_STATE,
    BAD_DESTINATION,

    COMPILATION_ERROR,
    RUNTIME_ERROR,
    SERIALIZATION_ERROR
  };

  ExecutionErrorMessage() = default;

  explicit ExecutionErrorMessage(Stage stage, Code code, std::string message)
    : stage_(stage)
    , code_(code)
    , message_(std::move(message))
  {}

  Stage stage() const
  {
    return stage_;
  }
  Code code() const
  {
    return code_;
  }
  std::string message() const
  {
    return message_;
  }

private:
  Stage       stage_;
  Code        code_;
  std::string message_;

  template <typename T, typename D>
  friend struct serializers::MapSerializer;
};

}  // namespace dmlf

namespace serializers {

template <typename D>
struct MapSerializer<fetch::dmlf::ExecutionErrorMessage, D>
{
public:
  using Type       = fetch::dmlf::ExecutionErrorMessage;
  using DriverType = D;

  static uint8_t const STAGE   = 1;
  static uint8_t const CODE    = 2;
  static uint8_t const MESSAGE = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &exec_err_msg)
  {
    auto const stage = static_cast<uint32_t>(exec_err_msg.stage_);
    auto const code  = static_cast<uint32_t>(exec_err_msg.code_);

    auto map = map_constructor(3);
    map.Append(STAGE, stage);
    map.Append(CODE, code);
    map.Append(MESSAGE, exec_err_msg.message_);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &exec_err_msg)
  {
    uint32_t stage, code;

    map.ExpectKeyGetValue(STAGE, stage);
    map.ExpectKeyGetValue(CODE, code);
    map.ExpectKeyGetValue(MESSAGE, exec_err_msg.message_);
    exec_err_msg.stage_ = static_cast<dmlf::ExecutionErrorMessage::Stage>(stage);
    exec_err_msg.code_  = static_cast<dmlf::ExecutionErrorMessage::Code>(code);
  }
};

}  // namespace serializers
}  // namespace fetch
