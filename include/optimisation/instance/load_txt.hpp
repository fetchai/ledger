#ifndef OPTIMISATION_INSTANCE_LOAD_TXT_HPP
#define OPTIMISATION_INSTANCE_LOAD_TXT_HPP

#include <string/trim.hpp>

#include <fstream>
#include <map>
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
  std::map<int, std::size_t> indices;
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

    if (indices.find(i) == indices.end()) indices[i] = k++;

    if (indices.find(j) == indices.end()) indices[j] = k++;

    c.i = indices[i];
    c.j = indices[j];
    couplings.push_back(c);
  }

  optimiser.Resize(k);

  for (auto &c : couplings) optimiser(c.i, c.j) = c.c;
  return true;
}
};
};

#endif
