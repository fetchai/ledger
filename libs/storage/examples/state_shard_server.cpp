//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "core/commandline/cli_header.hpp"
#include "network/service/server.hpp"
#include "storage/document_store.hpp"
#include "storage/document_store_protocol.hpp"
#include <iostream>
using namespace fetch::storage;

class StateShardService : public fetch::service::ServiceServer<fetch::network::TCPServer>
{
public:
  StateShardService(uint16_t port, fetch::network::NetworkManager tm) : ServiceServer(port, tm)
  {
    store_ = new RevertibleDocumentStore();
    store_->Load("a.db", "b.db", "c.db", "d.db", true);
    store_protocol_ = new RevertibleDocumentStoreProtocol(store_);
    this->Add(0, store_protocol_);
  }

  ~StateShardService()
  {
    // TODO(issue 14): Remove protocol
    delete store_protocol_;
    delete store_;
  }

private:
  RevertibleDocumentStore *        store_;
  RevertibleDocumentStoreProtocol *store_protocol_;
};

int main()
{
  fetch::logger.DisableLogger();

  fetch::network::NetworkManager tm(8);
  StateShardService              serv(8080, tm);
  tm.Start();

  std::string dummy;
  fetch::commandline::DisplayCLIHeader("Single state shard server");

  std::cout << "Press ENTER to quit" << std::endl;
  std::cin >> dummy;

  tm.Stop();

  return 0;
}
