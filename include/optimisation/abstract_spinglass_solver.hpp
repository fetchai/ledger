#ifndef OPTIMISATION_ABSTRACT_SPINGLASS_SOLVER_HPP
#define OPTIMISATION_ABSTRACT_SPINGLASS_SOLVER_HPP

namespace fetch {
namespace optimisers {

class AbstractSpinGlassSolver {
 public:
  virtual void Resize(std::size_t const &n, std::size_t const &max_connectivity = std::size_t(-1)) = 0;  
  virtual void Insert(std::size_t const &i, std::size_t const &j, cost_type const &c) = 0;
  virtual void Update(std::size_t const &i, std::size_t const &j, cost_type const &c) = 0;

};
  
};
};
#endif
