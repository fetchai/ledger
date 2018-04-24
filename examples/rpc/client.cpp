#define FETCH_DISABLE_COUT_LOGGING
#include"service_consts.hpp"
#include<iostream>
#include"serializer/referenced_byte_array.hpp"
#include"service/client.hpp"
#include"logger.hpp"
using namespace fetch::service;
using namespace fetch::byte_array;


int main() {
  
  // Client setup
  fetch::network::ThreadManager tm(2);  
  ServiceClient< fetch::network::TCPClient > client("localhost", 8080, &tm);

  client.OnLeave( []() {
      std::cout << "Goood bye!!" << std::endl;
      
    });

  
 
  tm.Start();

  
  std::this_thread::sleep_for( std::chrono::milliseconds(100) );
  
  std::cout << client.Call( MYPROTO,GREET, "Fetch" ).As<std::string>( ) << std::endl;  

  auto px = client.Call( MYPROTO,SLOWFUNCTION,"Greet"  );
  
  // Promises
  auto p1 = client.Call( MYPROTO,SLOWFUNCTION, 2, 7 );
  auto p2 = client.Call( MYPROTO,SLOWFUNCTION, 4, 3 );

//  client.WithDecorators(aes, ... ).Call( MYPROTO,SLOWFUNCTION, 4, 3 );
  
  
  if(!p1.is_fulfilled())
    std::cout << "p1 is not yet fulfilled" << std::endl;


  p1.Wait();
  
  // Converting to a type implicitly invokes Wait (as is the case for p2)
  std::cout << "Result is: " << int(p1) << " " << int(p2) << std::endl;

  try {
    // We called SlowFunction with wrong parameters
    // It will throw an exception
    std::cout << "Second result: " << int(px) << std::endl;
    
  } catch(fetch::serializers::SerializableException const &e) {
    
    std::cout << "Exception caught: " << e.what() << std::endl;
    
  }

  // Testing performance
  auto t_start = std::chrono::high_resolution_clock::now();
  std::vector< fetch::service::Promise > promises;

  std::size_t N = 10000;
  for(std::size_t i=0; i < N; ++i) {
    promises.push_back( client.Call( MYPROTO, ADD, 4, 3 ) );
  }
  fetch::logger.Highlight("DONE!");
  
  std::cout << "Waiting for last promise: " << promises.back().id() << std::endl;
  promises.back().Wait(false);
  
  std::size_t failed = 0, not_fulfilled = 0;  
  for(auto &p: promises) {
    if((p.has_failed()) || (p.is_connection_closed())) {
      ++failed;      
    }
    if(!p.is_fulfilled()) {
      ++not_fulfilled ;
    }    
  }
  std::cout << failed << " requests failed!" << std::endl;
  std::cout << not_fulfilled << " requests was not fulfilled!" << std::endl;  
  
  
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));  
  auto t_end = std::chrono::high_resolution_clock::now();
  
  std::cout << "Wall clock time passed: "
            << std::chrono::duration<double, std::milli>(t_end-t_start).count()
              << " ms\n";
  std::cout << "Time per call:: "
            << double(std::chrono::duration<double, std::milli>(t_end-t_start).count()) / double(N) * 1000.
            << " us\n";  

  
  // Benchmarking  
  tm.Stop();

  return 0;

}

int xmain() {


  fetch::network::ThreadManager tm(1);
  tm.Start(); // Started thread manager before client construction!    
  ServiceClient< fetch::network::TCPClient > client("localhost", 8080, &tm);
  client.OnLeave( []() {
      std::cout << "Goood bye!!" << std::endl;
    });

  auto promise = client.Call( MYPROTO,SLOWFUNCTION, 2, 7 );
  
  if(!promise.Wait(500)){ // wait 500 ms for a response
    std::cout << "no response from node: " << client.is_alive() <<  std::endl;
    promise = client.Call( MYPROTO,SLOWFUNCTION, 2, 7 );
  } else {
    std::cout << "response from node!" << std::endl << std::endl;
  }
  
  tm.Stop();
  
  return 0;
}
