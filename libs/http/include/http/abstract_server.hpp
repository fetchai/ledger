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

#include "core/byte_array/byte_array.hpp"
#include "core/mutex.hpp"
#include "http/request.hpp"

namespace fetch {
namespace http {

class AbstractHTTPServer
{
public:
  using handle_type = uint64_t;

  virtual void PushRequest(handle_type client, HTTPRequest req) = 0;

  static handle_type next_handle()
  {
    std::lock_guard<fetch::mutex::Mutex> lck(global_handle_mutex_);
    handle_type                          ret = global_handle_counter_;
    ++global_handle_counter_;
    return ret;
  }

private:
  static handle_type         global_handle_counter_;
  static fetch::mutex::Mutex global_handle_mutex_;
};

}  // namespace http
}  // namespace fetch
