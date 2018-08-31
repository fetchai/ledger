//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "network/service/client.hpp"
#include "core/logger.hpp"
#include "core/serializers/byte_array.hpp"
#include "service_consts.hpp"
#include <iostream>
using namespace fetch::service;
using namespace fetch::byte_array;

int main()
{

  // Client setup
  fetch::network::NetworkManager tm(2);

  tm.Start();
  {
    fetch::network::TCPClient connection(tm);
    connection.Connect("localhost", 8080);

    ServiceClient client(connection, tm);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  fetch::network::TCPClient connection(tm);
  connection.Connect("localhost", 8080);

  ServiceClient client(connection, tm);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  std::cout << client.Call(MYPROTO, GREET, "Fetch")->As<std::string>() << std::endl;

  auto px = client.Call(MYPROTO, SLOWFUNCTION, "Greet");

  // Promises
  auto p1 = client.Call(MYPROTO, SLOWFUNCTION, 2, 7);
  auto p2 = client.Call(MYPROTO, SLOWFUNCTION, 4, 3);
  auto p3 = client.Call(MYPROTO, SLOWFUNCTION);
  //  client.WithDecorators(aes, ... ).Call( MYPROTO,SLOWFUNCTION, 4, 3 );

  if (p1->IsWaiting()) std::cout << "p1 is not yet fulfilled" << std::endl;

  FETCH_LOG_PROMISE();
  p1->Wait();

  // Converting to a type implicitly invokes Wait (as is the case for p2)
  std::cout << "Result is: " << p1->As<int>() << " " << p2->As<int>() << std::endl;
  try
  {
    p3->Wait();
  }
  catch (fetch::serializers::SerializableException const &e)
  {
    std::cout << "Exception caught: " << e.what() << std::endl;
  }

  try
  {
    // We called SlowFunction with wrong parameters
    // It will throw an exception
    std::cout << "Second result: " << px->As<int>() << std::endl;
  }
  catch (fetch::serializers::SerializableException const &e)
  {
    std::cout << "Exception caught: " << e.what() << std::endl;
  }

  // Testing performance
  auto                                 t_start = std::chrono::high_resolution_clock::now();
  std::vector<fetch::service::Promise> promises;

  std::size_t N = 100000;
  for (std::size_t i = 0; i < N; ++i)
  {
    promises.push_back(client.Call(MYPROTO, ADD, 4, 3));
  }
  fetch::logger.Highlight("DONE!");

  std::cout << "Waiting for last promise: " << promises.back()->id() << std::endl;

  FETCH_LOG_PROMISE();
  promises.back()->Wait(false);

  std::size_t failed = 0, not_fulfilled = 0;
  for (auto &p : promises)
  {
    if (p->IsFailed())
    {
      ++failed;
    }
    if (p->IsWaiting())
    {
      ++not_fulfilled;
    }
  }
  std::cout << failed << " requests failed!" << std::endl;
  std::cout << not_fulfilled << " requests was not fulfilled!" << std::endl;

  std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  auto t_end = std::chrono::high_resolution_clock::now();

  std::cout << "Wall clock time passed: "
            << std::chrono::duration<double, std::milli>(t_end - t_start).count() << " ms\n";
  std::cout << "Time per call:: "
            << double(std::chrono::duration<double, std::milli>(t_end - t_start).count()) /
                   double(N) * 1000.
            << " us\n";

  // Benchmarking
  tm.Stop();

  return 0;
}

int xmain()
{

  fetch::network::NetworkManager tm(1);
  tm.Start();  // Started thread manager before client construction!

  fetch::network::TCPClient connection(tm);
  connection.Connect("localhost", 8080);

  ServiceClient client(connection, tm);

  auto promise = client.Call(MYPROTO, SLOWFUNCTION, 2, 7);

  FETCH_LOG_PROMISE();
  if (!promise->Wait(500, true))
  {  // wait 500 ms for a response
    std::cout << "no response from node: " << client.is_alive() << std::endl;
    promise = client.Call(MYPROTO, SLOWFUNCTION, 2, 7);
  }
  else
  {
    std::cout << "response from node!" << std::endl << std::endl;
  }

  tm.Stop();

  return 0;
}
