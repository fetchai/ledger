//#define FETCH_DISABLE_COUT_LOGGING
#include"serializer/referenced_byte_array.hpp"
#include"serializer/stl_types.hpp"
#include"serializer/byte_array_buffer.hpp"
#include "serializer/counter.hpp"
#include "random/lfg.hpp"

#include"service/server.hpp"
#include"serializer/referenced_byte_array.hpp"
#include"service/client.hpp"
#include"chain/transaction.hpp"

#include<chrono>
#include<vector>
#include<random>
using namespace fetch::serializers;
using namespace fetch::byte_array;
using namespace fetch::service;
using namespace std::chrono;
using namespace fetch;


typedef fetch::chain::Transaction transaction_type;

fetch::random::LaggedFibonacciGenerator<> lfg;
template< typename T>
void MakeString(T &str, std::size_t N = 256) {
  ByteArray entry;
  entry.Resize(N);
  
  for(std::size_t j=0; j < N; ++j) {
    entry[j] = uint8_t( lfg()  >> 19 );      
  }
  
  str = entry;
}


transaction_type NextTransaction()
 {
   static std::random_device rd;
   static std::mt19937 gen(rd());
   static std::uniform_int_distribution<uint32_t> dis(0, std::numeric_limits<uint32_t>::max());

   transaction_type trans;

   // Push on two 32-bit numbers so as to avoid multiple nodes creating duplicates
   trans.PushGroup( group_type( dis(gen) ) );
   trans.PushGroup( group_type( dis(gen) ) );
   trans.PushGroup( group_type( dis(gen) ) );
   trans.PushGroup( group_type( dis(gen) ) );
   trans.PushGroup( group_type( dis(gen) ) );


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


template< typename T, std::size_t N = 256 >
void MakeStringVector(std::vector< T >  &vec, std::size_t size) {

  for(std::size_t i=0; i < size; ++i ){
    T s;
    MakeString(s);
    vec.push_back( s );
  }
}

template< typename T, std::size_t N = 256 >
void MakeTransactionVector(std::vector< T >  &vec, std::size_t size) {

  for(std::size_t i=0; i < size; ++i ){
 
    vec.push_back( NextTransaction() );
  }
}


ByteArray TestString;

enum {
  SEND = 1,
  GET2 =2,
  SERVICE = 3
};
std::vector< transaction_type > TestData;

class Implementation {
public:
  void SendData(std::vector< transaction_type > const &data) {
    std::lock_guard< fetch::mutex::Mutex > lock( mutex_);
    if( data.size() != TestString.size() ){
      std::cout << "Something went wrong!" << std::endl;      
    }
    
//    copy = data;
  }
  
private:
  mutable fetch::mutex::Mutex mutex_;
  
  std::vector< transaction_type > copy;
};


class ServiceProtocol : public Protocol {
public:
  
  ServiceProtocol() : Protocol() {
    this->Expose(SEND, &impl_, &Implementation::SendData);
  }
private:
  Implementation impl_;
};


// And finanly we build the service
class MyCoolService : public ServiceServer< fetch::network::TCPServer > {
public:
  MyCoolService(uint16_t port, fetch::network::ThreadManager *tm) : ServiceServer(port, tm) {
    this->Add(SERVICE, new ServiceProtocol() );
  }
};


void StartClient() 
{
  fetch::network::ThreadManager tm;  
  ServiceClient< fetch::network::TCPClient > client("localhost", 8080, &tm);
  tm.Start();

  while(true) {
    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    std::cout << "Calling ..." << std::flush;

    auto p1 = client.Call( SERVICE, SEND, TestData );
    high_resolution_clock::time_point t0 = high_resolution_clock::now();        
    p1.Wait();
//    std::vector< transaction_type > data;
//    p1.As(data);
    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    duration<double> ts1 = duration_cast<duration<double>>(t1 - t0);    
    std::cout << " DONE: " << (TestData.size() ) << std::endl;
    if(TestData.size() > 0 ) {
      std::cout <<  ( double( TestData.size() ) / double( ts1.count() ) )  << " TX/s, "  << ts1.count() << " s" << std::endl;
    }
  }

  tm.Stop();
}

void RunTest(std::size_t tx_count) 
{
  MakeTransactionVector(TestData, tx_count);
  
  fetch::network::ThreadManager tm(8);  
  MyCoolService serv(8080, &tm);
  tm.Start();
  
  StartClient();
  
  tm.Stop();  

}


int main() 
{
  /*
  serializer_type ser;    
  ser <<   NextTransaction();  
  std::cout << "TX Size: " << ser.data().size() << std::endl;  
  */
  try {
    
    RunTest((1<<16) / (1903) + 400);
  }catch(std::exception const&e) {
    fetch::logger.PrintTimings();
  }
  
  return 0;
}
