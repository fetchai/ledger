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
// TODO(Singleton this class)
template <typename ArrayType, typename VariableType>
class SessionManager
{
 private:

  using FunctionSignature = typename VariableType::FunctionSignature;
  using VariablePtrType = std::shared_ptr<VariableType>;

  public:
  //  using VariableType = fetch::ml::layers::Layer<ArrayType>;

  // A counter of variables within the session
  std::size_t                      variable_counter = 0;
//  std::unordered_map<std::string, VariableType&>        all_variables{};
//  std::vector<VariableType>               top_sort{};

  std::unordered_map<std::string, VariablePtrType> all_variables;
  std::unordered_map<std::string, VariablePtrType> top_sort_map;
  std::vector<VariablePtrType> top_sort_vector;



  SessionManager() = default;


  VariablePtrType Variable(std::string const &variable_name = "", FunctionSignature const &b_fn = nullptr, bool is_leaf = true)
  {
    VariablePtrType var = std::make_shared<VariableType>();
    VariableSetup(var, variable_name, b_fn, is_leaf);
    return var;
  }

  VariablePtrType Variable(std::vector<std::size_t> in_shape, FunctionSignature const &f_fn = nullptr, std::string const &variable_name = "", FunctionSignature const &b_fn = nullptr, bool is_leaf = true)
  {
    VariablePtrType var = std::make_shared<VariableType>();
    var->SetData(ArrayType(in_shape));

    VariableSetup(var, variable_name, b_fn, is_leaf, f_fn);
    return var;
  }

  void VariableSetup(VariablePtrType var, std::string const &variable_name, FunctionSignature const &b_fn = nullptr, bool is_leaf = true, FunctionSignature const &f_fn = nullptr)
  {
    // variable ID (Implement pointer as ID)
    var->id() = variable_counter;
    ++variable_counter;

    // setup variable name
    if (variable_name.empty()) {var->SetVariableName("autoname_" + std::to_string(var->id()));}
    else {var->SetVariableName(variable_name + "_" + std::to_string(var->id()));}

    // Assign backward, forward functions and set leaf status
    assert(b_fn || is_leaf);
    var->SetBackwardFunction(b_fn);
    var->SetIsLeaf(is_leaf);
    if (f_fn)
    {
      var->SetForwardFunction(f_fn);
    }

    // initialise the variables gradients to zeros
    var->InitialiseGradients();

    // flag that the variable is ready for use
    var->initialised = true;

    // add to the map of all variables
    all_variables.insert({var->variable_name(), var});
  }

  void Forward(VariablePtrType var, std::string &output_name)
  {
    // output_name variable must exist
    assert(all_variables.find(output_name) != all_variables.end());

    // construct the computational graph
    BackwardGraph(all_variables.at(output_name));

    // there must be a path from the output variable to the input variable
    assert(top_sort_map.find(output_name) != top_sort_map.end());

    for (std::size_t i = 0; i < top_sort_vector.size(); --i)
    {
      top_sort_vector[i]->Forward(top_sort_vector[i]);
    }
  }

  void BackProp(VariablePtrType var, typename ArrayType::type const& lr)
  {
    var->ClearGradients();
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

  static VariablePtrType Zeroes(std::vector<std::size_t> const &new_shape, SessionManager &sess)
  {
    VariablePtrType ret = sess.Variable(new_shape);
    ret->data().SetAllZero();
    return ret;
  }
  static VariablePtrType Zeroes(std::size_t const &in_size, std::size_t const &out_size, SessionManager &sess)
  {
    std::vector<std::size_t> new_shape{in_size, out_size};
    VariablePtrType ret = sess.Variable(new_shape);
    ret->data().SetAllZero();
    return ret;
  }


  /**
   * builds a computation graph backwards
   * @param var
   * @return
   */
  void BackwardGraph(VariablePtrType var)
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
      top_sort_vector[i - 1]->Backward(top_sort_vector[i - 1]);
    }
  }

  /**
   * nagivate backwards through computational graph
   * @param vr
   * @param v_set
   */
  void TopSort(VariablePtrType var)
  {
    // check if we've added this variable already
    bool is_in = (top_sort_map.find(var->variable_name()) != top_sort_map.end());

    if ((!is_in) && (!var->is_leaf()))
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
