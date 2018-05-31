#include"network/tcp_server_echo.hpp"
#include<iostream>

int main(int argc, char *argv[])
{
  asio::io_service service;
  std::shared_ptr<asio::io_service::work> work = std::make_shared<asio::io_service::work>(service);
  std::vector<std::thread> threads;

  for (std::size_t i = 0; i < 5; ++i)
  {
    threads.emplace_back([&service]{ service.run(); std::cout << "Work finished." << std::endl; });
  }

  {
    std::cerr << "Starting tcp server" << std::endl;
    fetch::network::TcpServerEcho echo(service, 8080);

    char dummy;
    std::cout << "press any key to quit" << std::endl;
    std::cin >> dummy;
  }

  work.reset();
  service.stop();
  for(auto &i : threads)
  {
    i.join();
  }

  std::cout << "Finished" << std::endl;
  return 0;
}
