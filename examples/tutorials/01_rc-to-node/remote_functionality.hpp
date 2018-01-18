#ifndef REMOTE_FUNCTIONALITY_HPP
#define REMOTE_FUNCTIONALITY_HPP

#include<vector>
#include<string>
#include<iostream>

class RemoteFunctionality {
public:
  RemoteFunctionality(std::string const &node_info): node_info_(node_info) {  }
    
  void Connect(std::string address, uint16_t port) {
    std::cout << "Connecting to " << address << " " << port << std::endl;
  }
  
  void Disconnect(uint64_t handle) { }

  std::string get_info()  { return node_info_; }  
  std::vector< std::string > const& peers() const {
    return peers_;
  }

private:
  std::string node_info_;
  std::vector< std::string > peers_;
};

#endif
