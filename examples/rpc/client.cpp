#include<iostream>
#include"serializer/referenced_byte_array.hpp"
#include"rpc/service_client.hpp"
using namespace fetch::rpc;
using namespace fetch::byte_array;

int main() {
  // Client setup
  ServiceClient client("localhost", 8080);
  client.Start();

  std::this_thread::sleep_for( std::chrono::milliseconds(100) );
  
  std::cout << client.Call( "Mod","Greet", "Fetch" ).As<std::string>( ) << std::endl;  

  auto px = client.Call( "Mod","SlowFunction","Greet"  );
  
  // Promises
  auto p1 = client.Call( "Mod","SlowFunction", 2, 7 );
  auto p2 = client.Call( "Mod","SlowFunction", 4, 3 );
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
  fetch::rpc::Promise last_promise;

  std::size_t N = 1000;
  for(std::size_t i=0; i < N; ++i) {
    last_promise = client.Call( "Mod","Add", 4, 3 );
  }
    
  std::cout << "Waiting for last promise: " << last_promise.id() << std::endl;
  last_promise.Wait();
  auto t_end = std::chrono::high_resolution_clock::now();
  
  std::cout << "Wall clock time passed: "
            << std::chrono::duration<double, std::milli>(t_end-t_start).count()
              << " ms\n";
  std::cout << "Time per call:: "
            << std::chrono::duration<double, std::milli>(t_end-t_start).count() / N * 1000
            << " us\n";  
  
  // Benchmarking  
  client.Stop();

  return 0;

}
