#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include <iostream>
#include <unordered_set>
#include <vector>

namespace fetch {
namespace ml {

template <typename ArrayType, typename LayerType>
class SessionManager
{

public:
  //  using LayerType = fetch::ml::layers::Layer<ArrayType>;

  // A counter of variables within the session
  std::size_t                   variable_counter = 0;
  std::unordered_set<LayerType> vars_seen{};
  std::vector<LayerType>        top_sort{};

  void RegisterVariable(LayerType &var)
  {
    var.id = variable_counter;
    ++variable_counter;
  }

  /**
   * builds a computation graph backwards
   * @param var
   * @return
   */
  void BackwardGraph(LayerType &var)
  {
    TopSort(var);
    //  var.grad                         = T(var.data.shape());

    for (std::size_t i = top_sort.size(); i > 0; --i)
    {
      top_sort[i - 1].Backward();
    }
  }

  /**
   * nagivate backwards through computational graph
   * @param vr
   * @param v_set
   */
  void TopSort(LayerType &v)
  {
    bool is_in = (vars_seen.find(v) != vars_seen.end());

    if ((!is_in) && (!v.is_leaf))
    {
      vars_seen.insert(v);
      for (std::size_t i = 0; i < v.prev.size(); ++i)
      {
        TopSort(v.prev[i]);
      }
      top_sort.push_back(v);
    }
  }
};

}  // namespace ml
}  // namespace fetch
