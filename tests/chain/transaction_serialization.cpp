#include<iostream>
#include"chain/transaction.hpp"
#include"service/server.hpp"
#include"service/client.hpp"
#include"serializer/typed_byte_array_buffer.hpp"
#include"serializer/byte_array_buffer.hpp"
#include"serializer/stl_types.hpp"
#include"random/lfg.hpp"
#include"service/callable_class_member.hpp"
#include<vector>

using namespace fetch;
std::vector< std::string > words = {"squeak", "fork", "governor", "peace", "courageous", "support", "tight", "reject", "extra-small", "slimy", "form", "bushes", "telling", "outrageous", "cure", "occur", "plausible", "scent", "kick", "melted", "perform", "rhetorical", "good", "selfish", "dime", "tree", "prevent", "camera", "paltry", "allow", "follow", "balance", "wave", "curved", "woman", "rampant", "eatable", "faulty", "sordid", "tooth", "bitter", "library", "spiders", "mysterious", "stop", "talk", "watch", "muddle", "windy", "meal", "arm", "hammer", "purple", "company", "political", "territory", "open", "attract", "admire", "undress", "accidental", "happy", "lock", "delicious"}; 

fetch::random::LaggedFibonacciGenerator<> lfg;
std::mutex lfg_mutex;

std::string RandomString(std::size_t const &n) {
  lfg_mutex.lock();
  
  std::string ret = words[ lfg() & 63 ];
  for(std::size_t i=1; i < n;++i) {
    ret += " " + words[lfg() & 63];
  }

  lfg_mutex.unlock();
  return ret;
}

class Impl 
{
public:
  Impl() 
  {
  }
  
  std::vector< chain::Transaction > GetList(int i) 
  {
    fetch::logger.Highlight("Executing ", i );
    
    
    std::vector< chain::Transaction > list;
    
    for(std::size_t i=0; i < 1000; ++i) {
      chain::Transaction tx;
      tx.PushResource( RandomString(8) );
      tx.PushResource( RandomString(8) );
      tx.set_arguments( RandomString(24) );
      list.push_back(tx);   
    }
    fetch::logger.Highlight("Done executing ", i );
    return list;    
  }
};


double Test1() 
{
  std::vector< chain::Transaction > list;
  serializers::TypedByte_ArrayBuffer ss;

  for(std::size_t i=0; i < 1000; ++i) {
    chain::Transaction tx;
    tx.PushResource( RandomString(8) );
    tx.PushResource( RandomString(8) );
    tx.set_arguments( RandomString(24) );
    list.push_back(tx);   
  }
  
  std::cout << "Staring" << std::endl;
  
  auto start =   std::chrono::high_resolution_clock::now();
  ss << list;
  auto end =   std::chrono::high_resolution_clock::now();
  double total_time =  std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.;

  return total_time; 
}



int Test2() 
{
//  std::cout << "Took " << Test1() << "seconds" << std::endl;
  Impl x;
  fetch::service::CallableClassMember< Impl, std::vector< chain::Transaction >(int) > fnc(&x, &Impl::GetList);

  serializers::TypedByte_ArrayBuffer ss, params;
  auto start =   std::chrono::high_resolution_clock::now();
//  fnc(ss, params);
  auto end =   std::chrono::high_resolution_clock::now();
  double total_time =  std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.;
  std::cout << "Took " << total_time << "seconds" << std::endl;
  return 0;
   
}


class Prot : public Impl, public service::Protocol 
{
public:
  Prot(network::ThreadManager *thread_manager)
  {
    this->Expose(0, new fetch::service::CallableClassMember< Impl, std::vector< chain::Transaction >(int) >(this, &Impl::GetList) );    
  }
private:
  
};

class Service : public fetch::service::ServiceServer< fetch::network::TCPServer >
{
public:
  Service(uint16_t port, network::ThreadManager *thread_manager) :
    service::ServiceServer< fetch::network::TCPServer >(port, thread_manager),
    prot_(thread_manager)
  {
    this->Add(0, &prot_);    
  }
private:
  Prot prot_;
};

int main() 
{
  network::ThreadManager thread_manager(16);
  Service serv(8080, &thread_manager);
  thread_manager.Start();
  fetch::service::ServiceClient< fetch::network::TCPClient > client("localhost", 8080, &thread_manager );

  std::this_thread::sleep_for(std::chrono::milliseconds( 500 ));
  std::vector< service::Promise > promises;
  
  for(std::size_t i=0; i < 100; ++i) {
    auto p = client.Call(0,0, int(i));
    promises.push_back(p);
    
  }

  std::size_t i=0;
  for(auto &p: promises) {
    fetch::logger.Highlight("Waiting for ", i );    
    p.Wait();
    ++i;
    fetch::logger.PrintTimings(20);
    fetch::logger.PrintMutexTimings(20);    
  }
  
  
  thread_manager.Stop();
  
//  for(std::size_t i=0; i < 100; ++i)
//    Test2();
  return 0;
  

}
