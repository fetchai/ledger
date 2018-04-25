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

std::size_t txSize(const transaction_type &transaction)
{
  serializer_type ser;
  ser << transaction;
  return ser.size();
}

transaction_type NextTransaction(std::size_t groupsInTx = 5) {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<uint32_t> dis(
      0, std::numeric_limits<uint32_t>::max());

  transaction_type trans;

  for (std::size_t i = 0; i < groupsInTx; ++i)
  {
    trans.PushGroup(group_type(dis(gen)));
  }

  ByteArray sig1, sig2, contract_name, arg1;
  MakeString(sig1);
  MakeString(sig2);
  MakeString(contract_name);
  MakeString(arg1, 4 * 256);

  trans.PushSignature(sig1);
  trans.PushSignature(sig2);
  trans.set_contract_name(contract_name);
  trans.set_arguments(arg1);
  trans.UpdateDigest();

  return trans;
}

template <typename T, std::size_t N = 256>
void MakeStringVector(std::vector<T> &vec, std::size_t size) {
  for (std::size_t i = 0; i < size; ++i) {
    T s;
    MakeString(s);
    vec.push_back(s);
  }
}

template <typename T, std::size_t N = 256>
void MakeTransactionVector(std::vector<T> &vec, std::size_t groupsInTx, std::size_t txPerCall) {
  vec.clear();
  for (std::size_t i = 0; i < txPerCall; ++i) {
    vec.push_back(NextTransaction(groupsInTx));
  }
}

// Globals
ByteArray                     TestString;
std::vector<transaction_type> TestData;
enum { GET = 1, GET2 = 2, SERVICE = 3, SETUP = 4 };

class Implementation {
 public:
  std::vector<transaction_type> GetData()
  {
    std::cout << "Returning data" << std::endl;
    return TestData;
  }

  void Setup(std::size_t groupsInTx, std::size_t txPerCall)
  {
    std::cout << "Configuring: " << groupsInTx << ":" << txPerCall << std::endl;
    MakeTransactionVector(TestData, groupsInTx, txPerCall);
    std::cout << "Configured" << std::endl;
  }
};

class ServiceProtocol : public Protocol {
 public:
  ServiceProtocol() : Protocol() {
    this->Expose(GET, &impl_, &Implementation::GetData);
    this->Expose(SETUP, &impl_, &Implementation::Setup);
  }

 private:
  Implementation impl_;
};

// And finally we build the service
class BenchmarkService : public ServiceServer<fetch::network::TCPServer> {
 public:
  BenchmarkService(uint16_t port, fetch::network::ThreadManager *tm)
      : ServiceServer(port, tm) {
    this->Add(SERVICE, new ServiceProtocol());
  }
};

void RunTest(std::size_t groupsInTx, std::size_t txPerCall,
    const std::string &IP, uint16_t port) {

  fetch::network::ThreadManager tm;
  ServiceClient<fetch::network::TCPClient> client(IP, port, &tm);
  tm.Start();
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  uint64_t receivedTx = 0;
  auto p = client.Call(SERVICE, SETUP, groupsInTx, txPerCall);
  p.Wait();

  using time_p = high_resolution_clock::time_point;
  time_p t0 = high_resolution_clock::now();
  std::vector<transaction_type> data;

  while(receivedTx < 10000)
  {
    //std::cout << "Calling ..." << std::endl;

    auto p1 = client.Call(SERVICE, GET);
    p1.Wait();
    p1.As(data);
    receivedTx += txPerCall;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  time_p t1 = high_resolution_clock::now();
  tm.Stop();

  double seconds = duration_cast<duration<double>>(t1 - t0).count();

  std::cout << ">Groups:\t" << groupsInTx << "\tTX/rpc:\t" << txPerCall;

  std::cout << "\ttime:\t" << seconds << "\tTx/sec:\t" <<
    double(receivedTx)/seconds << "\tTx (bytes):\t" << txSize(data[0]) << std::endl;
}

int main(int argc, char *argv[])
{

  std::string IP;
  uint16_t    port = 8080; // Default for all benchmark tests

  if(argc > 1)
  {
    std::stringstream s(argv[1]);
    s >> IP;
  }
  else
  {
    fetch::network::ThreadManager tm(8);
    BenchmarkService serv(port, &tm);
    tm.Start();

    std::cout << "Master node, enter key to quit" << std::endl;
    std::cin >> port;
    tm.Stop();
    exit(0);
  }

  if(argc > 2)
  {
    std::stringstream s(argv[2]);
    s >> port;
  }

  std::cout << "IP:port " << IP << ":" << port << std::endl;

  for (std::size_t i = 0; i < 10; ++i)
  {
    for (std::size_t j = 0; j < 10; ++j)
    {
      std::size_t groupsInTx = i+1;
      std::size_t txPerCall = 1000 * (j+1);
      RunTest(groupsInTx, txPerCall, IP, port);
    }

    std::cout << std::endl;
  }

  return 0;
}
