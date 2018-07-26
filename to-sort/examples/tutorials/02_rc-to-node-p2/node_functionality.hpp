#pragma once

#include<map>
#include<vector>
#include<string>
#include<iostream>

class NodeFunctionality {
public:
  bool RegisterType(std::string name, std::string schema) {
    if(schemas_.find(name) != schemas_.end() ) return false;
    schemas_[name] = schema;
    std::cout << "Registered schema " << name << std::endl;
    return true;
  }
  
  int PushData(std::string name, std::vector< double > data) {
    if(schemas_.find(name) == schemas_.end()) return 0;
    
    if(data_.find(name) == data_.end() ) {
      std::cout << "Created new data set " << name << std::endl;
      data_[name] = std::vector< double >();
    }
    
    for(auto &a: data)
      data_[name].push_back(a);
    
    std::cout << "Added " << data.size() << " data points to " <<  name << std::endl;
    return 0;
  }
  
  std::vector< double > PurchaseData(std::string name) {
    if(schemas_.find(name) == schemas_.end()) return std::vector< double >();
    if(data_.find(name) == data_.end()) return std::vector< double >();
    std::cout << "Sold " << data_.size() << " data points of type " <<  name << std::endl;        
    return data_[name];
  }

private:
  std::map< std::string, std::string > schemas_;
  std::map< std::string, std::vector< double > > data_;  
};

