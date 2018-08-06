#pragma once

namespace fetch {
    namespace kernels {

        template <typename vector_register_type>
        struct Relu
        {
            void operator()(vector_register_type const &x, vector_register_type &y) const
            {
                const vector_register_type zero(typename vector_register_type::type(0));
                y = max(x, zero);
            }
        };

    }  // namespace kernels
}  // namespace fetch
