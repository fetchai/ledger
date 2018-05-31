#ifndef HELPER_FUNCTIONS_HPP
#define HELPER_FUNCTIONS_HPP

#include"random/lfg.hpp"
#include"byte_array/referenced_byte_array.hpp"
#include"serializers/counter.hpp"
#include"service/types.hpp"
#include"chain/transaction.hpp"

namespace fetch {
namespace common {

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
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<uint32_t> dis(
      0, std::numeric_limits<uint32_t>::max());

  T trans;

  trans.PushGroup(group_type(dis(gen)));

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

}
}

#endif
