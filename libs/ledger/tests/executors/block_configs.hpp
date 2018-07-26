#pragma once

#include <cstddef>
#include <iostream>
#include <vector>

struct BlockConfig
{
  using config_array_type = std::vector<BlockConfig>;

  std::size_t executors;
  std::size_t log2_lanes;
  std::size_t slices;

  std::size_t lanes() const { return 1u << log2_lanes; }

  static config_array_type const MAIN_SET;
  static config_array_type const REDUCED_SET;

  friend std::ostream &operator<<(std::ostream &     stream,
                                  BlockConfig const &config)
  {
    stream << "{ executors: " << config.executors
           << " lanes: " << config.lanes() << " slices: " << config.slices
           << " }";

    return stream;
  }
};

