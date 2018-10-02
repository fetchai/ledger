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

template <typename ArrayType, typename VariableType>
class SessionManager
{

public:
  //  using VariableType = fetch::ml::layers::Layer<ArrayType>;

  // A counter of variables within the session
  std::size_t                      variable_counter = 0;
  std::unordered_set<VariableType> vars_seen{};
  std::vector<VariableType>        top_sort{};

  SessionManager() = default;

  void RegisterVariable(std::size_t &id, std::string var_name)
  {
    id = variable_counter;
    ++variable_counter;
  }

  /**
   * builds a computation graph backwards
   * @param var
   * @return
   */
  void BackwardGraph(VariableType &var)
  {
    var.GradientSetOne();

    TopSort(var);

    std::cout << "TopSort finished" << std::endl;
    for (std::size_t i = top_sort.size(); i > 0; --i)
    {
      std::cout << "i: " << i << std::endl;
      std::cout << "begin backward on: " << top_sort[i - 1].variable_name() << std::endl;
      top_sort[i - 1].Backward();
    }
  }

  /**
   * nagivate backwards through computational graph
   * @param vr
   * @param v_set
   */
  void TopSort(VariableType &v)
  {
    bool is_in = (vars_seen.find(v) != vars_seen.end());
    std::cout << "is_in: " << is_in << std::endl;
    std::cout << "if: " << ((!is_in) && (!v.is_leaf)) << std::endl;

    if ((!is_in) && (!v.is_leaf))
    {
      vars_seen.insert(v);
      std::cout << "inserted: " << v._variable_name << std::endl;
      std::cout << "v.prev.size(): " << v.prev.size() << std::endl;
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
