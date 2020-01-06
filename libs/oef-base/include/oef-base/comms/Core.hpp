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

#include "network/fetch_asio.hpp"
#include <atomic>
#include <memory>
#include <mutex>

using asio::ip::tcp;

class Core
{
public:
  Core();
  virtual ~Core();

  void run();
  void stop();

  explicit operator asio::io_context *()
  {
    return context.get();
  }
  explicit operator asio::io_context &()
  {
    return *context;
  }

  std::shared_ptr<tcp::acceptor> makeAcceptor(unsigned short int port);

private:
  std::shared_ptr<asio::io_context> context;
  asio::io_context::work *          work;
};
