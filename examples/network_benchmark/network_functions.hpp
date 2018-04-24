#ifndef NETWORK_FUNCTIONS_H
#define NETWORK_FUNCTIONS_H
#include<random>
#include "logger.hpp"
namespace fetch
{
namespace network_benchmark
{

using transaction = chain::Transaction;

fetch::random::LaggedFibonacciGenerator<> lfg;

template< typename T>
void MakeString(T &str, std::size_t N = 256) {
  LOG_STACK_TRACE_POINT ;
  byte_array::ByteArray entry;
  entry.Resize(N);

  for(std::size_t j=0; j < N; ++j) {
    entry[j] = uint8_t( lfg()  >> 19 );
  }

  str = entry;
}

transaction NextTransaction()
{
  LOG_STACK_TRACE_POINT ;
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<uint32_t> dis(0, std::numeric_limits<uint32_t>::max());

  transaction trans;

  trans.PushGroup( group_type( dis(gen) ) );
  trans.PushGroup( group_type( dis(gen) ) );
  trans.PushGroup( group_type( dis(gen) ) );
  trans.PushGroup( group_type( dis(gen) ) );
  trans.PushGroup( group_type( dis(gen) ) );

  byte_array::ByteArray sig1, sig2, contract_name, arg1;
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


}
}

#endif /* NETWORK_FUNCTIONS_H */
