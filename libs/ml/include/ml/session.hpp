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
  SessionManager(double gradient_clip)
  {
    gradient_clip_ = gradient_clip;
  }

  /*
   * These methods construct and track variables
   */

  //  VariablePtrType Variable(std::vector<std::size_t> const &in_shape,
  //                           std::vector<std::size_t> const &grad_shape,
  //                           std::string const &             variable_name = "",
  //                           FunctionSignature const &       f_fn          = nullptr,
  //                           FunctionSignature const &b_fn = nullptr, bool is_leaf = true)
  //  {
  //    VariablePtrType var = std::make_shared<VariableType>();
  //    VariableSetup(var, in_shape, variable_name, b_fn, is_leaf, f_fn, grad_shape);
  //    return var;
  //  }
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
  VariablePtrType Variable(std::vector<std::size_t> const &in_shape,
                           std::string const &variable_name, FunctionSignature const &f_fn,
                           FunctionSignature const &b_fn, bool is_leaf)
  {
    VariablePtrType var = std::make_shared<VariableType>();
    VariableSetup(var, in_shape, variable_name, f_fn, b_fn, is_leaf, false, in_shape);
    return var;
  }
  void VariableSetup(VariablePtrType var, std::vector<std::size_t> in_shape,
                     std::string const &variable_name, FunctionSignature const &f_fn = nullptr,
                     FunctionSignature const &b_fn = nullptr, bool is_leaf = true,
                     bool requires_grad = false, std::vector<std::size_t> grad_shape = {})
  {
    var->SetData(ArrayType(in_shape));

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

  /*
   * These methods construct layers and track their internal variables
   */

  LayerPtrType Layer(std::size_t const &in_size, std::size_t const &out_size,
                     std::string layer_name = "")
  {
    return LayerSetup({in_size, out_size}, layer_name);
  }
  LayerPtrType Layer(std::vector<std::size_t> const &in_shape, std::string layer_name = "")
  {
    return LayerSetup(in_shape, layer_name);
  }
  LayerPtrType LayerSetup(std::vector<std::size_t> const &in_shape, std::string layer_name)
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
    VariableSetup(weights, in_shape, layer_name + "_weights", nullptr, nullptr, true, true, in_shape);

    VariablePtrType biases = std::make_shared<VariableType>();
    VariableSetup(biases, {1, in_shape[1]}, layer_name + "_biases", nullptr, nullptr, true, true, in_shape);

    LayerPtrType l = std::make_shared<LayerType>();
    l->weights()   = weights;
    l->biases()    = biases;
    l->Initialise(in_shape, weights, biases);
    return l;
  }

  /**
   * Interface function for users, returns a forward prediction
   * @param in_var
   * @param out_var
   * @return
   */
  ArrayType Predict(VariablePtrType in_var, VariablePtrType out_var)
  {
    Forward(in_var, out_var);
    return out_var->data();
  }
  void Forward(VariablePtrType in_var, VariablePtrType out_var)
  {
    Forward(in_var, out_var->variable_name());
  }
  void Forward(VariablePtrType in_var, std::string &output_name)
  {
    // output_name variable must exist
    assert(all_variables.find(output_name) != all_variables.end());

    // Conduct a topology sort
    top_sort_map.clear();
    top_sort_vector.clear();
    TopSort(all_variables.at(output_name), false);

    // there must be a path from the output variable to the input variable
    assert(top_sort_map.find(output_name) != top_sort_map.end());

    for (std::size_t i = 0; i < top_sort_vector.size(); ++i)
    {
      top_sort_vector[i]->Forward(top_sort_vector[i]);
    }
  }

  void BackProp(VariablePtrType input_var, VariablePtrType loss_var,
                typename ArrayType::Type const &lr, std::size_t nreps = 1)
  {

    for (std::size_t j = 0; j < nreps; ++j)
    {
      //      std::cout << "Forward TopSort: " << std::endl;
      Forward(input_var, loss_var->variable_name());

      ClearGradients(loss_var);
      //
      //      // TODO: l2 regularisation imlpementation
      //      // l2 reg
      //      double l2_norm_sum = 0;
      //      for (auto &cur_var : all_variables)
      //      {
      //        l2_norm_sum += fetch::math::L2Norm(cur_var.second->data());
      //      }
      //
      //      for (std::size_t i = 0; i < loss_var->data().size(); ++i)
      //      {
      //        loss_var->data()[i] += l2_norm_sum;
      //      }

      // calculate new gradients
      //      std::cout << "BackGraph TopSort: " << std::endl;
      BackwardGraph(loss_var);
      //
      //      std::cout << "BackwardGraph: " << std::endl;
      //      for (auto &cur_var : all_variables)
      //      {
      //        std::cout << "variable_name(): " << cur_var.second->variable_name() << std::endl;
      //        for (std::size_t idx = 0; idx < cur_var.second->data().size(); ++idx)
      //        {
      //          std::cout << "data: " << cur_var.second->data()[idx] << std::endl;
      //        }
      //        for (std::size_t idx = 0; idx < cur_var.second->grad().size(); ++idx)
      //        {
      //          std::cout << "grad: " << cur_var.second->grad()[idx] << std::endl;
      //        }
      //      }

      //      std::cout << "Gradient step TopSort: " << std::endl;
      // update weights
      top_sort_map.clear();
      top_sort_vector.clear();
      TopSort(loss_var, true);
      GradientStep(
          loss_var, lr,
          gradient_clip_);  // TODO: DON'T UPDATE THE DATA IN THE LOSS VAR OR THE INPUT VAR!!!!!!

//      std::cout << "loss: " << fetch::math::Sum(loss_var->data()) << std::endl;
    }
  }

  void ClearGradients(VariablePtrType loss_var)
  {
    // clear all gradients
    top_sort_map.clear();
    top_sort_vector.clear();
    TopSort(loss_var, true);
    for (std::size_t i = 0; i < top_sort_vector.size(); ++i)
    {
      top_sort_vector[i]->ClearGradients();
    }
  }

  void GradientStep(VariablePtrType loss_var, typename ArrayType::Type const &lr,
                    typename ArrayType::Type gradient_clip)
  {
    // TODO(private 277)
    for (std::size_t i = 0; i < top_sort_vector.size() - 1; ++i)
    {
      top_sort_vector[i]->GradientStep(lr, gradient_clip);
    }
  }

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
    VariablePtrType          ret = sess.Variable(new_shape, "zeroes");
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
    TopSort(var, false);

    // iterate through the necessary variables for gradient updating
    for (std::size_t i = top_sort_vector.size(); i > 0; --i)
    {
      top_sort_vector[i - 1]->Backward(top_sort_vector[i - 1]);
    }
  }

  /**
   * nagivate backwards through computational graph
   * if backward set to false, the check for requires_gradient will be dropped
   * @param vr
   * @param v_set
   */
  void TopSort(VariablePtrType var, bool gradient_step = false)
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
            TopSort(var->prev[i], gradient_step);
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
            TopSort(var->prev[i], gradient_step);
          }
          // pushing back to the vector after the recursive call ensures the right order
          top_sort_vector.push_back(var);
        }
      }
    }
  }
  void SetInput(LayerPtrType layer, VariablePtrType input)
  {
    layer->SetInput(input, *this);
  }

private:
  double gradient_clip_ = -1.0;  // negative values for gradient clip indicate clipping is off
};

}  // namespace ml
}  // namespace fetch
