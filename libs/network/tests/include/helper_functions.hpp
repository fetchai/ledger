#ifndef HELPER_FUNCTIONS_HPP
#define HELPER_FUNCTIONS_HPP

#include<random>
//#include"core/random/lfg.hpp"
//#include"core/byte_array/referenced_byte_array.hpp"
//#include"network/serializers/counter.hpp"
//#include"network/service/types.hpp"
//#include"chain/transaction.hpp"

namespace fetch {
namespace common {

uint32_t GetRandom()
{
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<uint32_t> dis(
      0, std::numeric_limits<uint32_t>::max());
  return dis(gen);
}

// Time related functionality
typedef std::chrono::high_resolution_clock::time_point time_point;

time_point TimePoint()
{
  return std::chrono::high_resolution_clock::now();
}

double TimeDifference(time_point t1, time_point t2)
{
  // If t1 before t2
  if(t1 < t2)
  {
    return std::chrono::duration_cast<std::chrono::duration<double>> (t2 - t1).count();
  }
  return std::chrono::duration_cast<std::chrono::duration<double>> (t1 - t2).count();
}

//  TODO: (`HUT`) : seperate helper functions by submodule
/*
class NoCopyClass
{
public:

  NoCopyClass(){}

  NoCopyClass(int val) :
    classValue_{val} { }

  NoCopyClass(NoCopyClass &rhs)             = delete;
  NoCopyClass &operator=(NoCopyClass& rhs)  = delete;
  NoCopyClass(NoCopyClass &&rhs)            = delete;
  NoCopyClass &operator=(NoCopyClass&& rhs) = delete;

  int classValue_ = 0;
};

template <typename T>
inline void Serialize(T &serializer, NoCopyClass const &b)
{
  serializer << b.classValue_;
}

template <typename T>
inline void Deserialize(T &serializer, NoCopyClass &b)
{
  serializer >> b.classValue_;
}

template <typename T>
void MakeString(T &str, std::size_t N = 4)
{
  static fetch::random::LaggedFibonacciGenerator<> lfg;
  byte_array::ByteArray entry;
  entry.Resize(N);

  for (std::size_t j = 0; j < N; ++j)
  {
    entry[j] = uint8_t(lfg() & 0xFF);
  }
  str = entry;
}

template <typename T>
std::size_t Size(const T &item)
{
  serializers::SizeCounter<service::serializer_type> counter;
  counter << item;
  return counter.size();
}

template <typename T>
T NextTransaction(std::size_t bytesToAdd = 0)
{
  T trans;

  trans.PushGroup(group_type(GetRandom()));

  byte_array::ByteArray sig1, sig2, contract_name, arg1;
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


template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

std::size_t Hash(fetch::byte_array::ConstByteArray const &arr)
{
    std::size_t hash = 2166136261;
    for (std::size_t i = 0; i < arr.size(); ++i)
    {
      hash = (hash * 16777619) ^ arr[i];
    }
    return hash;
}

void BlockUntilTime(uint64_t startTime)
{
  // get time as epoch, wait until that time to start
  std::time_t t = static_cast<std::time_t>(startTime);
  std::tm* timeout_tm = std::localtime(&t);

  time_t timeout_time_t = mktime(timeout_tm);
  std::chrono::system_clock::time_point timeout_tp =
    std::chrono::system_clock::from_time_t(timeout_time_t);

  std::this_thread::sleep_until(timeout_tp);
}
*/

} // namespace commmon

/*
namespace network_benchmark
{

// Transactions are packaged up into blocks and referred to using a hash
typedef fetch::chain::Transaction         transaction_type;
typedef std::size_t                       block_hash;
typedef std::vector<transaction_type>     block_type;
typedef std::pair<block_hash, block_type> network_block;

} */

}

#endif
