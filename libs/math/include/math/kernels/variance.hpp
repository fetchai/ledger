#ifndef KERNELS_VARIANCE_HPP
#define KERNELS_VARIANCE_HPP

namespace fetch {
  namespace kernels {

    template< typename type, typename vector_register_type >
    struct Variance {
      Variance(type const&m, type const&r)
        : mean(m), rec(r) { }
      void operator()(vector_register_type const &a, vector_register_type &c) const {
        c = a - mean;
        c = rec * c * c;
      }
      vector_register_type mean;        
      vector_register_type rec;
    };
  }
}

#endif
