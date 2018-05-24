#include <cstdlib>
#include <iostream>
#include "serializers/stl_types.hpp"
#include"network/tcp_client.hpp"
using namespace fetch::network;

class Client : public TCPClient {
public:
  Client(std::string const &host,
    std::string const &port,
      ThreadManager &tmanager) :
    TCPClient(host, port, tmanager )
  {
  }

  ~Client() {
    this->Close();
  }

  void PushMessage(message_type const &value) override
  {

  }

  void ConnectionFailed() override
  {

  }

private:

};

template< std::size_t N = 1>
void TestCase1(int argc, char* argv[]) {
  {
      std::cerr << "Create tm" << std::endl;
      ThreadManager tmanager(N);
      std::cerr << "Starting" << std::endl;
      tmanager.Start();
      std::cerr << "Stopping" << std::endl;
  }

  {
      std::cerr << "Create tm" << std::endl;
      ThreadManager tmanager(1);
      std::cerr << "Starting" << std::endl;
      tmanager.Start();
      std::cerr << "Stopping" << std::endl;

      tmanager.Post([&tmanager]() { tmanager.Stop(); });           
      tmanager.Stop();
  }
  //  std::this_thread::sleep_for(std::chrono::seconds(1));
  std::cout << std::endl << std::endl;
}

template< std::size_t N = 1>
void TestCase2(int argc, char* argv[]) {
  std::cout << "TEST CASE 2" << std::endl;    
  for (std::size_t i = 0; i < 1000; ++i)
    {
      std::cerr << "Create tm" << std::endl;
      ThreadManager tmanager(N);
      std::cerr << "Starting" << std::endl;
      tmanager.Start();
      std::cerr << "Stopping" << std::endl;
      tmanager.Post([&tmanager]() { tmanager.Stop(); });

    }
}

template< std::size_t N = 1>
void TestCase3(int argc, char* argv[]) {
  std::cout << "TEST CASE 3" << std::endl;
    for (std::size_t i = 0; i < 120; ++i)
    {
      std::cerr << "Create tm" << std::endl;
      ThreadManager tmanager(N);
      std::cerr << "Starting" << std::endl;
      tmanager.Start();

      for (std::size_t j = 0; j < 4; ++j)
      {
        std::cerr << "Create client" << std::endl;
        Client client(argv[1], argv[2], tmanager);
        std::cerr << "Created client: " << i << ":" << j << std::endl << std::endl;
      }
      if(i%2) tmanager.Stop();
      if(i%3) tmanager.Stop();
      if(i%5) tmanager.Stop();            
      std::this_thread::sleep_for(std::chrono::microseconds(10000));      
      std::cerr << "Stopping" << std::endl;      
      std::cerr << "Finished loop\n\n" << std::endl;
      
    }
}

template< std::size_t N = 1>
void TestCase4(int argc, char* argv[]) {
  std::cout << "TEST CASE 4" << std::endl;
    for (std::size_t i = 0; i < 120; ++i)
    {
      std::cerr << "Create tm" << std::endl;
      ThreadManager tmanager(N);
      std::cerr << "Starting" << std::endl;
      tmanager.Start();

      for (std::size_t j = 0; j < 4; ++j)
      {
        std::cerr << "Create client" << std::endl;
        Client client(argv[1], argv[2], tmanager);
        std::cerr << "Created client: " << i << ":" << j << std::endl << std::endl;
      }
      
      tmanager.Stop();

      for (std::size_t j = 0; j < 4; ++j)
      {
        std::cerr << "Create client after" << std::endl;
        Client client(argv[1], argv[2], tmanager);
        std::cerr << "Created client: " << i << ":" << j << std::endl << std::endl;
      }
      tmanager.Start();
      if(i%2) tmanager.Stop();
      if(i%3) tmanager.Stop();
      if(i%5) tmanager.Stop();                  
      std::this_thread::sleep_for(std::chrono::microseconds(10000));      
      std::cerr << "Stopping" << std::endl;      
      std::cerr << "Finished loop\n\n" << std::endl;
      
    }
}

template< std::size_t N = 1>
void TestCase5(int argc, char* argv[]) {
  std::cout << "TEST CASE 5" << std::endl;
    for (std::size_t i = 0; i < 120; ++i)
    {
      std::vector< Client > clients;
      std::cerr << "Create tm" << std::endl;
      ThreadManager tmanager(N);
      std::cerr << "Starting" << std::endl;
      tmanager.Start();

      for (std::size_t j = 0; j < 4; ++j)
      {
        std::cerr << "Create client" << std::endl;
        clients.push_back( Client(argv[1], argv[2], tmanager) );
        std::cerr << "Created client: " << i << ":" << j << std::endl << std::endl;
      }
      
      tmanager.Stop();

      for (std::size_t j = 0; j < 4; ++j)
      {
        std::cerr << "Create client after" << std::endl;
        clients.push_back( Client(argv[1], argv[2], tmanager) );
        std::cerr << "Created client: " << i << ":" << j << std::endl << std::endl;
      }
      tmanager.Start();
      if(i%2) tmanager.Stop();
      if(i%3) tmanager.Stop();
      if(i%5) tmanager.Stop();                  
      std::this_thread::sleep_for(std::chrono::microseconds(10000));      
      std::cerr << "Stopping" << std::endl;      
      std::cerr << "Finished loop\n\n" << std::endl;
      
    }
}


int main(int argc, char* argv[]) {
  
  if (argc != 3) {
    std::cerr << "Usage: client <host> <port>\n";
    return 1;
  }
  TestCase1<1>(argc, argv);
  TestCase2<1>(argc, argv);    
  TestCase3<1>(argc, argv);
  TestCase4<1>(argc, argv);
  TestCase5<1>(argc, argv);            

  TestCase1<10>(argc, argv);
  TestCase2<10>(argc, argv);    
  TestCase3<10>(argc, argv);
  TestCase4<10>(argc, argv);
  TestCase5<10>(argc, argv);            
  
  return 0;

}
