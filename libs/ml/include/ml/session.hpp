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

  SessionManager() = default;
  explicit SessionManager(typename ArrayType::Type gradient_clip)
  {
    gradient_clip_ = gradient_clip;
  }

  /**
   * Define a variable in the computational graph
   * @param in_shape
   * @param variable_name
   * @param requires_grad
   * @param grad_shape
   * @return
   */
  VariablePtrType Variable(std::vector<std::size_t> const &in_shape,
                           std::string const &variable_name = "", bool requires_grad = false,
                           std::vector<std::size_t> grad_shape = {})
  {
    if (grad_shape.empty())
    {
      grad_shape = in_shape;
    }

    VariablePtrType var = std::make_shared<VariableType>();
    VariableSetup(var, in_shape, variable_name, nullptr, nullptr, true, requires_grad, grad_shape);
    return var;
  }
  /**
   * Define a variable in the computational graph
   * @param in_shape
   * @param variable_name
   * @param f_fn
   * @param b_fn
   * @param is_leaf
   * @return
   */
  VariablePtrType Variable(std::vector<std::size_t> const &in_shape,
                           std::string const &variable_name, FunctionSignature const &f_fn,
                           FunctionSignature const &b_fn, bool is_leaf)
  {
    VariablePtrType var = std::make_shared<VariableType>();
    VariableSetup(var, in_shape, variable_name, f_fn, b_fn, is_leaf, false, in_shape);
    return var;
  }

  /**
   * Define a layer in the neural net
   * @param in_size
   * @param out_size
   * @param activation
   * @param layer_name
   * @return
   */
  LayerPtrType Layer(std::size_t const &in_size, std::size_t const &out_size,
                     std::string activation = "LeakyRelu", std::string layer_name = "")
  {
    return LayerSetup({in_size, out_size}, activation, layer_name);
  }
  /**
   * Define a layer in the neural net
   * @param in_shape
   * @param activation
   * @param layer_name
   * @return
   */
  LayerPtrType Layer(std::vector<std::size_t> const &in_shape, std::string activation = "LeakyRelu",
                     std::string layer_name = "")
  {
    return LayerSetup(in_shape, activation, layer_name);
  }

  /**
   * Interface function for users, returns a forward prediction
   * @param in_var
   * @param out_var
   * @return
   */
  ArrayType Predict(VariablePtrType in_var, VariablePtrType out_var)
  {
    //    auto n_examples_to_predict = in_var->shape()[0];

    TopSort_NoGradientUpdate(out_var->variable_name());
    Forward(in_var, out_var);
    return out_var->data();
  }

  /**
   * The backpropagation algorithm.
   * @param input_var the entry point to the neural net for a forward pass (i.e. the input data)
   * @param loss_var the exit point from the neural net for a forward pass (i.e. the loss function)
   * @param lr learning rate
   * @param nreps number of repetitions
   */
  void BackProp(VariablePtrType input_var, VariablePtrType loss_var,
                typename ArrayType::Type const &lr, std::size_t nreps = 1)
  {

    TopSort_NoGradientUpdate(loss_var->variable_name());
    TopSort_GradientUpdate(loss_var->variable_name());

    for (std::size_t j = 0; j < nreps; ++j)
    {
      Forward(input_var, loss_var);
      ClearGradients(loss_var);

      // calculate gradients
      BackwardGraph(loss_var);

      // apply gradients
      for (std::size_t i = 0; i < top_sort_vector_g.size() - 1; ++i)
      {
        top_sort_vector_g[i]->GradientStep(lr, gradient_clip_);
      }
    }
  }

  /**
   * Clears all gradients in the computational graph
   * @param loss_var
   */
  void ClearGradients(VariablePtrType loss_var)
  {
    // clear all gradients
    for (auto &cur_var : all_variables)
    {
      cur_var.second->ClearGradients();
    }
  }

  /**
   * Helper function returning a variable full of zeroes
   * @param new_shape
   * @param sess
   * @return
   */
  static VariablePtrType Zeroes(std::vector<std::size_t> const &new_shape, SessionManager &sess)
  {
    VariablePtrType ret = sess.Variable(new_shape, "zeroes");
    ret->data().SetAllZero();
    return ret;
  }
  static VariablePtrType Zeroes(std::size_t const &in_size, std::size_t const &out_size,
                                SessionManager &sess)
  {
    std::vector<std::size_t> new_shape{in_size, out_size};
    return SessionManager::Zeroes(new_shape, sess);
  }

  /**
   * Sets the layer input
   * @param layer
   * @param input
   */
  void SetInput(LayerPtrType layer, VariablePtrType input)
  {
    layer->SetInput(input, *this);
  }

private:
  double gradient_clip_ = -1.0;  // negative values for gradient clip indicate clipping is off
  std::unordered_map<std::string, VariablePtrType> top_sort_map_ng;
  std::vector<VariablePtrType>                     top_sort_vector_ng;
  std::unordered_map<std::string, VariablePtrType> top_sort_map_g;
  std::vector<VariablePtrType>                     top_sort_vector_g;

  void Forward(VariablePtrType in_var, VariablePtrType out_var)
  {
    // output_name variable must exist
    assert(all_variables.find(out_var->variable_name()) != all_variables.end());

    // there must be a path from the output variable to the input variable
    assert(top_sort_map_ng.find(out_var->variable_name()) != top_sort_map_ng.end());

    for (std::size_t i = 0; i < top_sort_vector_ng.size(); ++i)
    {
      top_sort_vector_ng[i]->Forward(top_sort_vector_ng[i]);
    }
  }

  /**
   * call all variables' derivative functions to propagate gradients backwards
   * @param var
   * @return
   */
  void BackwardGraph(VariablePtrType var)
  {
    // all gradients are 0 by default, so set initial gradients to one
    var->GradientSetOne();

    // iterate through the necessary variables for gradient updating
    for (std::size_t i = top_sort_vector_ng.size(); i > 0; --i)
    {
      top_sort_vector_ng[i - 1]->Backward(top_sort_vector_ng[i - 1]);
    }
  }

  /**
   * Topological sorting excluding the leaf nodes requiring gradient upates
   * @param output_name
   */
  void TopSort_NoGradientUpdate(std::string &output_name)
  {
    // Conduct a topology sort
    top_sort_map_ng.clear();
    top_sort_vector_ng.clear();
    TopSort(all_variables.at(output_name), top_sort_map_ng, top_sort_vector_ng, false);
  }
  /**
   * Topological sorting including the leaf nodes requiring gradient upates
   * @param output_name
   */
  void TopSort_GradientUpdate(std::string &output_name)
  {
    // Conduct a topology sort
    top_sort_map_g.clear();
    top_sort_vector_g.clear();
    TopSort(all_variables.at(output_name), top_sort_map_g, top_sort_vector_g, true);
  }

  /**
   * nagivate backwards through computational graph
   * if gradient_step set to false, the check for requires_gradient will be dropped
   * @param vr
   * @param v_set
   */
  void TopSort(VariablePtrType var, std::unordered_map<std::string, VariablePtrType> &top_sort_map,
               std::vector<VariablePtrType> &top_sort_vector, bool gradient_step = false)
  {
    // check if we've added this variable already
    bool is_in = (top_sort_map.find(var->variable_name()) != top_sort_map.end());

    if (!is_in)  // check if we've already added this variable
    {
      if (gradient_step)  // some leaf nodes require gradient updates
      {
        if (var->requires_grad())
        {
          // we can update the map immediately
          top_sort_map.insert({var->variable_name(), var});
          for (std::size_t i = 0; i < var->prev.size(); ++i)
          {
            TopSort(var->prev[i], top_sort_map, top_sort_vector, gradient_step);
          }
          // pushing back to the vector after the recursive call ensures the right order
          top_sort_vector.push_back(var);
        }
      }
      else  // non-leaf nodes must have forward and backward functions
      {
        if (!var->is_leaf())
        {
          // we can update the map immediately
          top_sort_map.insert({var->variable_name(), var});
          for (std::size_t i = 0; i < var->prev.size(); ++i)
          {
            TopSort(var->prev[i], top_sort_map, top_sort_vector, gradient_step);
          }
          // pushing back to the vector after the recursive call ensures the right order
          top_sort_vector.push_back(var);
        }
      }
    }
  }

  /**
   * Method initialising a new layer in the neural net
   * @param in_shape
   * @param activation
   * @param layer_name
   * @return
   */
  LayerPtrType LayerSetup(std::vector<std::size_t> const &in_shape, std::string activation,
                          std::string layer_name)
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
    VariableSetup(weights, in_shape, layer_name + "_weights", nullptr, nullptr, true, true,
                  in_shape);

    VariablePtrType biases = std::make_shared<VariableType>();
    VariableSetup(biases, {1, in_shape[1]}, layer_name + "_biases", nullptr, nullptr, true, true,
                  {1, in_shape[1]});

    LayerPtrType l = std::make_shared<LayerType>();
    l->weights()   = weights;
    l->biases()    = biases;
    l->ActivationSetup(activation);
    l->Initialise(in_shape, weights, biases);
    return l;
  }

  /**
   * Define a variable in the computational graph
   * @param var
   * @param in_shape
   * @param variable_name
   * @param f_fn
   * @param b_fn
   * @param is_leaf
   * @param requires_grad
   * @param grad_shape
   */
  void VariableSetup(VariablePtrType var, std::vector<std::size_t> in_shape,
                     std::string const &variable_name, FunctionSignature const &f_fn = nullptr,
                     FunctionSignature const &b_fn = nullptr, bool is_leaf = true,
                     bool requires_grad = false, std::vector<std::size_t> grad_shape = {})
  {
    auto initial_data = ArrayType(in_shape);
    initial_data.SetAllZero();
    var->SetData(initial_data);

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
    var->SetIsLeaf(is_leaf, requires_grad);
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
};

}  // namespace ml
}  // namespace fetch
