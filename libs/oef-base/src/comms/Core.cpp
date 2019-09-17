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

#include "oef-base/comms/Core.hpp"
#include "oef-base/monitoring/Counter.hpp"

#include <iostream>

static Counter loopstart("mt-core.network.asio.run");
static Counter loopstop("mt-core.network.asio.ended");

Core::Core()
{
  context_ = std::make_shared<asio::io_context>();
  work_    = new asio::io_context::work(*context_);
}

Core::~Core()
{
  stop();
}

void Core::run()
{
  loopstart++;
  context_->run();
  loopstop++;
}

void Core::stop()
{
  // TODO: work life-cycle is assymetric to context
  delete work_;
  context_->stop();
}

std::shared_ptr<tcp::acceptor> Core::makeAcceptor(uint16_t port)
{
  return std::make_shared<tcp::acceptor>(*context_, tcp::endpoint(tcp::v4(), port));
}
