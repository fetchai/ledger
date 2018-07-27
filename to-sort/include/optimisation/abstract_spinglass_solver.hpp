#pragma once

namespace fetch {
namespace optimisers {

class AbstractSpinGlassSolver {
 public:
  typedef double cost_type;

  virtual void Resize(std::size_t const &n,
                      std::size_t max_connectivity = std::size_t(-1)) = 0;
  virtual void Insert(std::size_t const &i, std::size_t const &j,
                      cost_type const &c) = 0;
  virtual void Update(std::size_t const &i, std::size_t const &j,
                      cost_type const &c) = 0;
};
}
}
