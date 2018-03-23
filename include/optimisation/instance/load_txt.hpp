#ifndef OPTIMISATION_INSTANCE_LOAD_TXT_HPP
#define OPTIMISATION_INSTANCE_LOAD_TXT_HPP

#include <string/trim.hpp>

#include <fstream>
#include <unordered_map>
#include <sstream>
#include <string>
#include <vector>

namespace fetch {
namespace optimisers {

template <typename T>
bool Load(T &optimiser, std::string const &filename) {
  std::fstream fin(filename, std::ios::in);
  if (!fin) return false;

  std::string line;

  struct Coupling {
    std::size_t i = -1, j = -1;
    double c = 0;
  };

  std::vector<Coupling> couplings;
  std::unordered_map<int, std::size_t> indices;
  std::unordered_map< int, std::size_t > connectivity;
  
  int k = 0;

  while (fin) {
    std::getline(fin, line);

    int p = line.find('#');
    if (p != std::string::npos) line = line.substr(0, p);
    string::Trim(line);
    if (line == "") continue;
    std::stringstream ss(line);
    Coupling c;
    int i, j;

    ss >> i >> j >> c.c;
    if ((i == -1) || (j == -1)) break;

    if (indices.find(i) == indices.end()) {
      indices[i] = k++;
      connectivity[i] = 0;
    }
    

    if (indices.find(j) == indices.end()) {
      indices[j] = k++;
      connectivity[j] = 0;
     
    }
    

    c.i = indices[i];
    c.j = indices[j];
    
    ++connectivity[i];
    ++connectivity[j];
    
    couplings.push_back(c);
  }
  
  std::size_t connect_count = 0;
  for(auto &p: connectivity) {
    if(connect_count < p.second) connect_count = p.second;
  }
  
  optimiser.Resize(k, connect_count);

  for (auto &c : couplings) optimiser(c.i, c.j) = c.c;
  return true;
}
};
};

#endif
