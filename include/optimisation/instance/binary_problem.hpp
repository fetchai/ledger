#ifndef OPTIMISATION_INSTANCE_BINARY_PROBLEM_HPP
#define OPTIMISATION_INSTANCE_BINARY_PROBLEM_HPP
#include <memory/rectangular_array.hpp>
#include <memory/shared_array.hpp>
#include<unordered_set>
#include<vector>
namespace fetch {
namespace optimisers {
  class BinaryProblem {
  public:
    typedef double cost_type;

    void Resize(std::size_t const &n, std::size_t const &max_connectivity = std::size_t(-1)) {
      couplings_.Resize(n, n);
      coupling_sum_ = memory::SharedArray< cost_type >(n);
      
      for (std::size_t i = 0; i < couplings_.size(); ++i) couplings_[i] = 0;
      for(std::size_t i=0; i < coupling_sum_.size(); ++i) coupling_sum_[i] = 0;
      couples_to_.resize(n);
      
      size_ = n;
      energy_offset_ = 0;
    }
    
    void Insert(std::size_t const &i, std::size_t const &j, cost_type const &c) {
      std::size_t A = i;
      std::size_t B = j;
      if(j<i) {
        A = j;
        B = i;
      }
      couplings_.Set(A, B , c);
      if(A != B) {
        couples_to_[A].insert(B);
        couples_to_[B].insert(A);
        
        coupling_sum_[A] += c;        
        coupling_sum_[B] += c;

        energy_offset_ += c / 4.; // One fourth due to symmetry
      } else {
        energy_offset_ +=  c / 2.;
      }
    }

    template< typename T >
    void ProgramSpinGlassSolver(T &annealer) const {
      std::size_t max_connectivity=0;
       
      for(auto const &cc: couples_to_) {
        if(cc.size() > max_connectivity) {
          max_connectivity = cc.size();
        }
      }
      std::cout << "Max Connectivity: " << max_connectivity << std::endl;
      
      annealer.Resize(size_, max_connectivity);
      
      for(std::size_t i=0; i < size_; ++i) {
        cost_type field = - 0.5*(couplings_(i,i) + 0.5*coupling_sum_[i]);
        
        annealer.Insert( i, i, field);
                         
        for(std::size_t j=i + 1; j < size_; ++j) {
          if( couplings_(i,j) != 0 ) {
            annealer.Insert( i, j, 0.25 * couplings_(i,j) );
          }
          
        }

      }
    }

    cost_type energy_offset() const {
      return energy_offset_;
    }
  private:
    std::size_t size_ = 0;
    cost_type energy_offset_ = 0;
    std::vector< std::unordered_set< uint64_t > > couples_to_;    
    memory::RectangularArray<cost_type> couplings_;
    memory::SharedArray< cost_type > coupling_sum_;
  };
}
}
  

#endif
