#pragma once

namespace fetch {
    namespace kernels {

        template <typename vector_register_type>
        struct Relu
        {
            void operator()(vector_register_type const &x, vector_register_type &y) const
            {
                static const vector_register_type zero(0);
                if (x > zero)
                {
                    y = x;
                }
                else
                {
                    y = 0;
                }
//                y = (x > zero) ? x : zero;
            }
        };

    }  // namespace kernels
}  // namespace fetch
