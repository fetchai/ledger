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

#include "core/random/lfg.hpp"
#include "core/serializers/byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/counter.hpp"
#include "core/serializers/stl_types.hpp"
#include "network/service/server.hpp"
#include "network/service/service_client.hpp"
#include "network/uri.hpp"

#include "helper_functions.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chain/transaction_serialization.hpp"
#include "network/muddle/muddle.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"
#include <chrono>
#include <iomanip>
#include <random>
#include <vector>
using namespace fetch::serializers;
using namespace fetch::byte_array;
using namespace fetch::service;
using namespace fetch::common;
using namespace std::chrono;
using namespace fetch;

#include "network/test-helpers/muddle_test_client.hpp"
#include "network/test-helpers/muddle_test_definitions.hpp"
#include "network/test-helpers/muddle_test_server.hpp"

using transaction_type = fetch::ledger::VerifiedTransaction;

std::size_t                         sizeOfTxMin = 0;  // base size of Tx
ByteArray                           TestString;
std::vector<transaction_type>       TestData;
const std::vector<transaction_type> RefVec;

template <typename T, std::size_t N = 256>
std::size_t MakeTransactionVector(std::vector<T> &vec, std::size_t payload, std::size_t txPerCall)
{
  vec.clear();
  for (std::size_t i = 0; i < txPerCall - 1; ++i)
  {
    vec.push_back(
        NextTransaction<transaction_type>((payload - Size(RefVec)) / txPerCall - sizeOfTxMin));
  }
  vec.push_back(NextTransaction<transaction_type>(payload - Size(RefVec) -
                                                  (txPerCall - 1) * Size(vec[0]) - sizeOfTxMin));

  return payload;
}

enum
{
  PULL    = 1,
  PUSH    = 2,
  SERVICE = 2,
  SETUP   = 3
};

class Implementation
{
public:
  const std::vector<transaction_type> &PullData()
  {
    return TestData;
  }

  void PushData(std::vector<transaction_type> &data)
  {
    volatile std::vector<transaction_type> hold = std::move(data);
  }

  std::size_t Setup(std::size_t payload, std::size_t txPerCall, bool /*isMaster*/)
  {
    return MakeTransactionVector(TestData, payload, txPerCall);
  }
};

class ServiceProtocol : public Implementation, public Protocol
{
public:
  ServiceProtocol()
    : Protocol()
  {
    this->Expose(PULL, (Implementation *)this, &Implementation::PullData);
    this->Expose(PUSH, (Implementation *)this, &Implementation::PushData);
    this->Expose(SETUP, (Implementation *)this, &Implementation::Setup);
  }
};

class BenchmarkService : public ServiceServer<fetch::network::TCPServer>
{
public:
  BenchmarkService(uint16_t port, fetch::network::NetworkManager tm)
    : ServiceServer(port, tm)
  {
    this->Add(SERVICE, &serviceProtocol_);
  }

private:
  ServiceProtocol serviceProtocol_;
};

std::ostringstream finalResult;
double             mbps_running       = 0;
double             mbps_running_count = 0;
double             mbps_peak          = 0;
double             mbps_min           = 0;

void RunTest(std::size_t payload, std::size_t txPerCall, const std::string &IP, uint16_t port,
             bool isMaster, bool pullTest)
{

  if (payload / txPerCall < sizeOfTxMin)
  {
    return;
  }

  std::size_t txData       = 0;
  std::size_t rpcCalls     = 0;
  std::size_t setupPayload = 0;

  TClientPtr client = MuddleTestClient::CreateTestClient(IP, port);

  if (!pullTest)
  {
    setupPayload = MakeTransactionVector(TestData, payload, txPerCall);
  }
  else
  {
    auto p = client->Call(SERVICE, SETUP, payload, txPerCall, isMaster);

    FETCH_LOG_PROMISE();
    p->Wait();
    p->As(setupPayload);
  }

  if (0 == setupPayload)
  {
    std::cerr << "Failed to setup for payload: " << payload << " TX/call: " << txPerCall
              << std::endl;
    return;
  }

  std::vector<transaction_type>     data;
  std::size_t                       stopCondition = std::size_t(1 * pow(10, 6));
  high_resolution_clock::time_point t0, t1;

  if (pullTest)
  {
    t0 = high_resolution_clock::now();

    while (payload * rpcCalls < stopCondition)
    {
      auto p1 = client->Call(SERVICE, PULL);

      FETCH_LOG_PROMISE();
      p1->Wait();
      p1->As(data);
      txData += txPerCall;
      rpcCalls++;
    }

    t1 = high_resolution_clock::now();
  }
  else
  {
    t0 = high_resolution_clock::now();

    while (payload * rpcCalls < stopCondition)
    {
      auto p1 = client->Call(SERVICE, PUSH, TestData);

      FETCH_LOG_PROMISE();
      p1->Wait();
      txData += txPerCall;
      rpcCalls++;
    }

    t1 = high_resolution_clock::now();
  }

  double seconds = duration_cast<duration<double>>(t1 - t0).count();
  double mbps    = (double(rpcCalls * setupPayload * 8) / seconds) / 1000000;

  mbps_running += mbps;
  mbps_running_count += 1;
  if (mbps > mbps_peak)
  {
    mbps_peak = mbps;
  }
  if (mbps < mbps_min || mbps_min == 0)
  {
    mbps_min = mbps;
  }

  std::ostringstream result;
  result << std::left << std::setw(10) << double(setupPayload) / 1000 << std::left << std::setw(10)
         << txPerCall << std::left << std::setw(10) << double(txData) / seconds << std::left
         << std::setw(10) << mbps << std::left << std::setw(10) << seconds << std::endl;

  std::cout << result.str();
  finalResult << result.str();
}

int main(int argc, char *argv[])
{
  sizeOfTxMin = Size(NextTransaction<transaction_type>(0));
  std::cout << "Base tx size: " << sizeOfTxMin << std::endl;

  std::string                    IP;
  uint16_t                       port     = 8080;  // Default for all benchmark tests
  bool                           pullTest = true;
  fetch::network::NetworkManager tm{"NetMgr", 8};
  std::thread                    benchmarkThread;

  if (argc > 1)
  {
    std::stringstream s(argv[1]);
    s >> IP;
  }

  if (argc > 2)
  {
    std::string       result;
    std::stringstream s(argv[2]);
    s >> result;
    pullTest = result.compare("--push") == 0 ? false : true;
  }

  std::cout << "test IP:port " << pullTest << " " << IP << ":" << port << std::endl;

  if (IP.size() == 0 || IP.compare("localhost") == 0)
  {
    std::cout << "Starting server" << std::endl;

    // TODO(issue 28): refactor closures to use explicit copy
    benchmarkThread = std::thread([=]() {
      fetch::network::NetworkManager networkManager{"NetMgr", 8};
      BenchmarkService               serv(port, networkManager);
      networkManager.Start();
      std::string dummy;
      std::cin >> dummy;
      networkManager.Stop();
    });
  }

  if (IP.size() != 0)
  {
    std::cout << std::left << std::setw(10) << "Pay_kB" << std::left << std::setw(10) << "TX/rpc"
              << std::left << std::setw(10) << "Tx/sec" << std::left << std::setw(10) << "Mbps"
              << std::left << std::setw(10) << "time" << std::endl;

    for (std::size_t i = 0; i <= 10; ++i)
    {
      for (std::size_t j = 0; j <= 20; ++j)
      {
        std::size_t payload   = 100000 * (1 << i);
        std::size_t txPerCall = 100 * (1 << j);

        RunTest(payload, txPerCall, IP, port, true, pullTest);
      }
      std::cout << std::endl;
      finalResult << std::endl;
    }

    // std::cout << finalResult.str();
    std::cout << "Average Mb/s: " << mbps_running / mbps_running_count << std::endl;
    std::cout << "Peak Mb/s: " << mbps_peak << std::endl;
    std::cout << "Min Mb/s: " << mbps_min << std::endl;
  }

  tm.Stop();
  if (benchmarkThread.joinable())
  {
    std::cout << "Press key to exit" << std::endl;
    benchmarkThread.join();
  }

  return 0;
}
