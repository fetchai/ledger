#pragma once

#include "math/tensor.hpp"

#include <memory>
#include <utility>


class MNISTLoader
{

 public:
  MNISTLoader();

  unsigned int size() const;
  bool IsDone() const;
  void Reset();
  void Display(std::shared_ptr<fetch::math::Tensor<float>> const &data) const;

  std::pair<unsigned int, std::shared_ptr<fetch::math::Tensor<float>>> GetNext(std::shared_ptr<fetch::math::Tensor<float>> buffer);

 private:
  unsigned int cursor_;
  unsigned int size_;
  unsigned char ** data_;
  unsigned char * labels_;
};
