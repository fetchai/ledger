#ifndef KERNELS_STANDARD_DEVIATION_HPP
#define KERNELS_STANDARD_DEVIATION_HPP

namespace fetch {
  namespace kernels {

    template< typename type,  typename vector_register_type >
    struct StandardDeviation {
      StandardDeviation(type const&m, type const&r)
        : mean(m), rec(r) { }
      void operator()(vector_register_type const &a, vector_register_type &c) const {
        c = a - mean;
        c = rec * c * c;
        c = sqrt( c );
      }
      vector_register_type mean;        
      vector_register_type rec;
    };

  }
}

#endif
