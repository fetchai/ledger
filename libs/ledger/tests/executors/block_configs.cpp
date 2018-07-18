#include "block_configs.hpp"

BlockConfig::config_array_type const BlockConfig::MAIN_SET{
    {  1, 0,    1 }
  , {  1, 5,    1 }
  , {  1, 0,   32 }
  , {  4, 2,   64 }
  , {  8, 3,  128 }
  , { 16, 5,  128 }
  , { 32, 7,  256 }
  , { 64, 7, 1024 }
};

BlockConfig::config_array_type const BlockConfig::REDUCED_SET{
    {  1, 0,  1 }
  , {  1, 0, 32 }
  , {  4, 2, 64 }
  , {  8, 3, 64 }
};
