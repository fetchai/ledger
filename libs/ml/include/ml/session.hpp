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
#include <unordered_map>
#include <vector>
#include <memory>

namespace fetch {
namespace ml {

template <typename ArrayType, typename VariableType>
class SessionManager
{

public:
  //  using VariableType = fetch::ml::layers::Layer<ArrayType>;

  // A counter of variables within the session
  std::size_t                      variable_counter = 0;
//  std::unordered_map<std::string, VariableType&>        all_variables{};
//  std::vector<VariableType>               top_sort{};

  std::unordered_map<std::string, std::shared_ptr<VariableType>> all_variables;
  std::unordered_map<std::string, std::shared_ptr<VariableType>> top_sort_map;
  std::vector<std::shared_ptr<VariableType>> top_sort_vector;



  SessionManager() = default;

  void RegisterVariable(std::shared_ptr<VariableType> var)
  {
    var->id() = variable_counter;
    ++variable_counter;
    if (var->variable_name() == "") {var->variable_name() = "autoname_" + std::to_string(var->id());}
    else {var->variable_name() += "_" + std::to_string(var->id());}

    all_variables.insert({var->variable_name(), var});
  }

  void Forward(VariableType &var, std::string &output_name)
  {
    // output_name variable must exist
    assert(all_variables.find(output_name) != all_variables.end());

    // construct the computational graph
    BackwardGraph(all_variables.at(output_name));

    // there must be a path from the output variable to the input variable
    std::cout << var.variable_name() << std::endl;

    std::cout << "top_sort_vars_seen.size(): " << top_sort_map.size() << std::endl;
    for (auto const& i:top_sort_map)
    {
      std::cout << "i.first: " << i.first << std::endl;
    }
    std::cout << "var.variable_name(): " << var.variable_name() << std::endl;

    assert(top_sort_map.find(output_name) != top_sort_map.end());

    for (std::size_t i = top_sort_vector.size(); i > 0; --i)
    {
      top_sort_vector[i - 1]->Forward();
    }
  }

  void BackProp(VariableType &var, typename ArrayType::type const& lr)
  {
    BackwardGraph(var);
    GradientStep(lr);
  }

  void GradientStep(typename ArrayType::type const& lr)
  {
    for ( auto var : top_sort_vector)
    {
      var->GradientStep(lr);
    }
  }

  /**
   * builds a computation graph backwards
   * @param var
   * @return
   */
  void BackwardGraph(VariableType &var)
  {
    BackwardGraph(var.shared_from_this());
  }
  void BackwardGraph(std::shared_ptr<VariableType> var)
  {
    // all gradients are 0 by default, so set initial gradients to one
    var->GradientSetOne();

    // Conduct a topology sort
    top_sort_map.clear();
    top_sort_vector.clear();
    TopSort(var);

    // iterate through the necessary variables for gradient updating
    for (std::size_t i = top_sort_vector.size(); i > 0; --i)
    {
      top_sort_vector[i - 1]->Backward();
    }
  }

  /**
   * nagivate backwards through computational graph
   * @param vr
   * @param v_set
   */
  void TopSort(std::shared_ptr<VariableType> var)
  {
    // check if we've added this variable already
    bool is_in = (top_sort_map.find(var->variable_name()) != top_sort_map.end());

    if ((!is_in) && (!var->is_leaf))
    {
      // we can update the map immediately
      top_sort_map.insert({var->variable_name(), var});

      for (std::size_t i = 0; i < var->prev.size(); ++i)
      {
        TopSort(var->prev[i]);
      }

      // pushing back to the vector after the recursive call ensures the right order
      top_sort_vector.push_back(var);
    }
  }
};

}  // namespace ml
}  // namespace fetch
