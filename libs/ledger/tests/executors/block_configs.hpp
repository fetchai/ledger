#ifndef FETCH_BLOCK_CONFIGS_HPP
#define FETCH_BLOCK_CONFIGS_HPP

#include <cstddef>
#include <iostream>
#include <vector>

struct BlockConfig {
  using config_array_type = std::vector<BlockConfig>;

  std::size_t executors;
  std::size_t lanes;
  std::size_t slices;

  static config_array_type const MAIN_SET;

  friend std::ostream &operator<<(std::ostream &stream, BlockConfig const &config) {
    stream << "{ executors: " << config.executors
           << " lanes: " << config.lanes
           << " slices: " << config.slices
           << " }";

    return stream;
  }
};

#endif //FETCH_BLOCK_CONFIGS_HPP
