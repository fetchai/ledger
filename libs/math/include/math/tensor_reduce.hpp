#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "math/base_types.hpp"
#include "math/tensor_slice_iterator.hpp"

#include <cassert>

namespace fetch {
    namespace math {

// need to forward declare
        template <typename T, typename C>
        class Tensor;

        template <typename F, typename T, typename C>
        inline void Reduce(SizeType axis, F function, const Tensor<T, C> &a, Tensor<T, C> &b)
        {
            assert(b.shape().at(axis) == 1);
            for (SizeType i = 0; i < a.shape().size(); i++)
            {
                if (i != axis)
                {
                    assert(a.shape().at(i) == b.shape().at(i));
                }
            }

            if (axis == 0)
            {
                auto it_a  = a.cbegin();
                auto it_b = b.begin();

                while (it_a.is_valid())
                {
                    for (SizeType j{0}; j < a.shape().at(0); ++j)
                    {

                        function(*it_a,*it_b);
                        ++it_a;
                    }
                    ++it_b;
                }
            }
            else
            {
                auto it_a  = a.Slice().cbegin();
                auto it_b = b.Slice().begin();

                it_a.PermuteAxes(0, axis);
                it_b.PermuteAxes(0, axis);

                while (it_a.is_valid())
                {
                    for (SizeType j{0}; j < a.shape().at(0); ++j)
                    {

                        (*it_b) = function(*it_a);
                        ++it_a;
                    }
                    ++it_b;
                }
            }
        }



    }  // namespace math
}  // namespace fetch
