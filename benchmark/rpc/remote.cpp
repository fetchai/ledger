#define FETCH_DISABLE_COUT_LOGGING
#include "random/lfg.hpp"
#include "serializer/byte_array_buffer.hpp"
#include "serializer/counter.hpp"
#include "serializer/referenced_byte_array.hpp"
#include "serializer/stl_types.hpp"

#include "chain/transaction.hpp"
#include "serializer/referenced_byte_array.hpp"
#include "service/client.hpp"
#include "service/server.hpp"

#include <chrono>
#include <random>
#include <vector>
using namespace fetch::serializers;
using namespace fetch::byte_array;
using namespace fetch::service;
using namespace std::chrono;
using namespace fetch;

typedef fetch::chain::Transaction transaction_type;

fetch::random::LaggedFibonacciGenerator<> lfg;
template <typename T>
void MakeString(T &str, std::size_t N = 256) {
  ByteArray entry;
  entry.Resize(N);

  for (std::size_t j = 0; j < N; ++j) {
    entry[j] = uint8_t(lfg() >> 19);
  }
  str = entry;
}

template <typename T>
std::size_t Size(const T &item)
{
  serializer_type ser;
  ser << item;
  return ser.size();
}

std::size_t sizeOfTxMin   = 0; // base size of Tx

transaction_type NextTransaction(std::size_t bytesToAdd) {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<uint32_t> dis(
      0, std::numeric_limits<uint32_t>::max());

  transaction_type trans;

  trans.PushGroup(group_type(dis(gen)));

  ByteArray sig1, sig2, contract_name, arg1;
  MakeString(sig1);
  MakeString(sig2);
  MakeString(contract_name);
  MakeString(arg1, 1 + bytesToAdd);

  trans.PushSignature(sig1);
  trans.PushSignature(sig2);
  trans.set_contract_name(contract_name);
  trans.set_arguments(arg1);
  trans.UpdateDigest();

  return trans;
}

template <typename T>
void MakeStringVector(std::vector<T> &vec, std::size_t size) {
  for (std::size_t i = 0; i < size; ++i) {
    T s;
    MakeString(s);
    vec.push_back(s);
  }
}

ByteArray                           TestString;
std::vector<transaction_type>       TestData;
const std::vector<transaction_type> RefVec;

template <typename T, std::size_t N = 256>
std::size_t MakeTransactionVector(std::vector<T> &vec, std::size_t payload, std::size_t txPerCall) {
  vec.clear();
  for (std::size_t i = 0; i < txPerCall-1; ++i) {
    vec.push_back(NextTransaction(payload/txPerCall - sizeOfTxMin));
  }
  vec.push_back(NextTransaction(payload - Size(RefVec)
        - (txPerCall-1)*Size(vec[0]) - sizeOfTxMin));

  return payload;
}

enum { PULL = 1, PUSH = 2, SERVICE = 2, SETUP = 3 };

class Implementation {
 public:
  const std::vector<transaction_type> &PullData()
  {
    return TestData;
  }

  void PushData(std::vector<transaction_type> &data)
  {
    //std::cout << "Received push" << std::endl;
    volatile std::vector<transaction_type> hold = std::move(data);
    //std::cout << "Bye" << std::endl;
  }

  std::size_t Setup(std::size_t payload, std::size_t txPerCall, bool isMaster)
  {
    return MakeTransactionVector(TestData, payload, txPerCall);
  }
};

class ServiceProtocol : public Implementation, public Protocol {
 public:
  ServiceProtocol() : Protocol() {
    this->Expose(PULL, (Implementation*)this, &Implementation::PullData);
    this->Expose(PUSH, (Implementation*)this, &Implementation::PushData);
    this->Expose(SETUP, (Implementation*)this, &Implementation::Setup);
  }
};

class BenchmarkService : public ServiceServer<fetch::network::TCPServer> {
 public:
  BenchmarkService(uint16_t port, fetch::network::ThreadManager *tm)
      : ServiceServer(port, tm) {
    this->Add(SERVICE, &serviceProtocol);
  }

private:
  ServiceProtocol serviceProtocol;
};

void RunTest(std::size_t payload, std::size_t txPerCall,
    const std::string &IP, uint16_t port, bool isMaster, bool pullTest)
{

  if(payload/txPerCall < sizeOfTxMin) { return; }

  std::size_t txData       = 0;
  std::size_t rpcCalls     = 0;
  std::size_t setupPayload = 0;
  fetch::network::ThreadManager tm;
  ServiceClient<fetch::network::TCPClient> client(IP, port, &tm);
  tm.Start();

  while(!client.is_alive())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  if(!pullTest)
  {
    setupPayload = MakeTransactionVector(TestData, payload, txPerCall);
  }
  else
  {
    auto p = client.Call(SERVICE, SETUP, payload, txPerCall, isMaster);
    p.Wait();
    p.As(setupPayload);
  }

  if(0 == setupPayload)
  {
    std::cerr << "Failed to setup for payload: " <<
      payload << " TX/call: " << txPerCall << std::endl;
    tm.Stop();
    return;
  }

  std::vector<transaction_type> data;
  std::size_t stopCondition = 1 * pow(10, 6);
  high_resolution_clock::time_point t0, t1;

  if(pullTest)
  {
    t0 = high_resolution_clock::now();

    while(payload*rpcCalls < stopCondition)
    {
      auto p1 = client.Call(SERVICE, PULL);
      p1.Wait();
      p1.As(data);
      txData += txPerCall;
      rpcCalls++;
    }

    t1 = high_resolution_clock::now();
  }
  else
  {
    t0 = high_resolution_clock::now();

    while(payload*rpcCalls < stopCondition)
    {
      auto p1 = client.Call(SERVICE, PUSH, TestData);
      p1.Wait();
      txData += txPerCall;
      rpcCalls++;
    }

    t1 = high_resolution_clock::now();
  }

  tm.Stop();
  double seconds = duration_cast<duration<double>>(t1 - t0).count();
  double mbps = (double(rpcCalls*setupPayload*8)/seconds)/1000000;

  std::cout << std::left << std::setw(10)
            << double(setupPayload)/1000 << std::left << std::setw(10)
            << txPerCall                 << std::left << std::setw(10)
            << double(txData)/seconds    << std::left << std::setw(10)
            << mbps                      << std::left << std::setw(10)
            << seconds << std::endl;

}

int main(int argc, char *argv[])
{
  sizeOfTxMin = Size(NextTransaction(0));
  std::cout << "Base tx size: " << sizeOfTxMin << std::endl;

  std::string IP;
  uint16_t    port = 8080; // Default for all benchmark tests
  bool        pullTest = true;
  fetch::network::ThreadManager tm(8);

  if(argc > 1)
  {
    std::stringstream s(argv[1]);
    s >> IP;
  }

  if(argc > 2)
  {
    std::string result;
    std::stringstream s(argv[2]);
    s >> result;
    pullTest = result.compare("--push") == 0 ? false : true;
  }

  std::cout << "test IP:port " << pullTest << " " << IP << ":" << port << std::endl;

  if(IP.size() == 0)
  {
    BenchmarkService serv(port, &tm);
    tm.Start();
    std::cin >> port;
    tm.Stop();
    return 0;
  }

  std::cout << std::left << std::setw(10)
            << "Pay_kB" << std::left << std::setw(10)
            << "TX/rpc" << std::left << std::setw(10)
            << "Tx/sec" << std::left << std::setw(10)
            << "Mbps" << std::left << std::setw(10)
            << "time" << std::endl;

  for (std::size_t i = 0; i <= 10; ++i)
  {
    for (std::size_t j = 0; j <= 16; ++j)
    {
      std::size_t payload   = 200000  * (1<<i);
      std::size_t txPerCall = 100     * (1<<j);

      RunTest(payload, txPerCall, IP, port, true, pullTest);
    }
    std::cout << std::endl;
  }

  tm.Stop();
  return 0;
}
