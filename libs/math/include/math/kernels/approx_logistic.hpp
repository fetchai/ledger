#ifndef KERNELS_APPROX_LOGISTIC_HPP
#define KERNELS_APPROX_LOGISTIC_HPP

namespace fetch {
  namespace kernels {

    template< typename vector_register_type >
    struct ApproxLogistic {
      void operator() (vector_register_type const &x, vector_register_type &y) const {
        static const vector_register_type one(1);
        y = one + approx_exp( -x );
        y = one / y; 
      }
    };

  }
}

#endif
