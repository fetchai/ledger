#pragma once

class NDArrayView
{
 public:
  std::vector<std::size_t> from;
  std::vector<std::size_t> to;
  std::vector<std::size_t> step;

};