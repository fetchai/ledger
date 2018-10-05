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

#include "ml/layers/layers.hpp"
#include <iostream>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace fetch {
namespace ml {
// TODO(private 271)

template <typename A, typename V>
class SessionManager
{
private:
  using ArrayType         = A;
  using VariableType      = V;
  using VariablePtrType   = std::shared_ptr<VariableType>;
  using LayerType         = layers::Layer<ArrayType>;
  using LayerPtrType      = std::shared_ptr<LayerType>;
  using FunctionSignature = typename VariableType::FunctionSignature;

public:
  // A counter of variables within the session
  std::size_t                                      variable_counter = 0;  // TODO(private 272)
  std::size_t                                      layer_counter    = 0;
  std::unordered_map<std::string, VariablePtrType> all_variables;
  std::unordered_map<std::string, VariablePtrType> top_sort_map;
  std::vector<VariablePtrType>                     top_sort_vector;

  SessionManager() = default;

  /*
   * These methods construct and track variables
   */

  VariablePtrType Variable(std::string const &      variable_name = "",
                           FunctionSignature const &b_fn = nullptr, bool is_leaf = true)
  {
    VariablePtrType var = std::make_shared<VariableType>();
    VariableSetup(var, variable_name, b_fn, is_leaf);
    return var;
  }
  VariablePtrType Variable(std::vector<std::size_t>        in_shape,
                           std::vector<std::size_t> const &grad_shape,
                           std::string const &             variable_name = "",
                           FunctionSignature const &       f_fn          = nullptr,
                           FunctionSignature const &b_fn = nullptr, bool is_leaf = true)
  {
    VariablePtrType var = std::make_shared<VariableType>();
    var->SetData(ArrayType(in_shape));

    VariableSetup(var, variable_name, b_fn, is_leaf, f_fn, grad_shape);
    return var;
  }
  VariablePtrType Variable(std::vector<std::size_t> in_shape, std::string const &variable_name = "",
                           FunctionSignature const &f_fn = nullptr,
                           FunctionSignature const &b_fn = nullptr, bool is_leaf = true)
  {
    VariablePtrType var = std::make_shared<VariableType>();
    var->SetData(ArrayType(in_shape));

    VariableSetup(var, variable_name, b_fn, is_leaf, f_fn);
    return var;
  }

  void VariableSetup(VariablePtrType var, std::string const &variable_name,
                     FunctionSignature const &b_fn = nullptr, bool is_leaf = true,
                     FunctionSignature const &f_fn       = nullptr,
                     std::vector<std::size_t> grad_shape = {})
  {
    // variable ID (Implement pointer as ID)
    var->id() = variable_counter;
    ++variable_counter;

    // setup variable name
    if (variable_name.empty())
    {
      var->SetVariableName("autoname_" + std::to_string(var->id()));
    }
    else
    {
      var->SetVariableName(variable_name + "_" + std::to_string(var->id()));
    }

    // Assign backward, forward functions and set leaf status
    assert(b_fn || is_leaf);
    var->SetBackwardFunction(b_fn);
    var->SetIsLeaf(is_leaf);
    if (f_fn)
    {
      var->SetForwardFunction(f_fn);
    }

    // initialise the variables gradients to zeros
    var->InitialiseGradients(grad_shape);

    // flag that the variable is ready for use
    var->initialised = true;

    // add to the map of all variables
    all_variables.insert({var->variable_name(), var});
  }

  /*
   * These methods construct layers and track their internal variables
   */

  LayerPtrType Layer(std::size_t const &in_size, std::size_t const &out_size,
                     std::string const &layer_name = "")
  {
    return LayerSetup({in_size, out_size}, layer_name);
  }
  LayerPtrType Layer(std::vector<std::size_t> const &in_shape, std::string const &layer_name = "")
  {
    return LayerSetup(in_shape, layer_name);
  }
  LayerPtrType LayerSetup(std::vector<std::size_t> const &in_shape, std::string const &layer_name)
  {
    //    layer_counter->id() = layer_counter;
    if (layer_name.empty())
    {
      layer_name = "autoname_" + std::to_string(layer_counter);
    }
    else
    {
      layer_name = layer_name + "_" + std::to_string(layer_counter);
    }
    ++layer_counter;

    VariablePtrType weights = std::make_shared<VariableType>();
    VariableSetup(weights, layer_name + "_weights", nullptr, true);

    VariablePtrType biases = std::make_shared<VariableType>();
    VariableSetup(biases, layer_name + "_biases", nullptr, true);

    LayerType l = Layer();
    l.Initialise(in_shape, weights, biases);
  }

  void Forward(VariablePtrType in_var, VariablePtrType out_var)
  {
    Forward(in_var, out_var->variable_name());
  }
  void Forward(VariablePtrType in_var, std::string &output_name)
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

  void BackProp(VariablePtrType input_var, VariablePtrType output_var,
                typename ArrayType::type const &lr, std::size_t nreps = 1)
  {

    for (std::size_t j = 0; j < nreps; ++j)
    {
      Forward(input_var, output_var->variable_name());

      // Conduct a top sort and clear all gradients
      top_sort_map.clear();
      top_sort_vector.clear();
      TopSort(output_var);
      for (std::size_t i = top_sort_vector.size(); i > 0; --i)
      {
        top_sort_vector[i - 1]->ClearGradients();
      }

      //    var->ClearGradients();
      BackwardGraph(output_var);
      GradientStep(lr);
    }
  }

  void GradientStep(typename ArrayType::type const &lr)
  {
    for (auto var : top_sort_vector)
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
  static VariablePtrType Zeroes(std::size_t const &in_size, std::size_t const &out_size,
                                SessionManager &sess)
  {
    std::vector<std::size_t> new_shape{in_size, out_size};
    VariablePtrType          ret = sess.Variable(new_shape);
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
