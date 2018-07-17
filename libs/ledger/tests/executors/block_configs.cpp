#include "block_configs.hpp"

BlockConfig::config_array_type const BlockConfig::MAIN_SET{
    {  1,   1,    1 }
  , {  1,  32,    1 }
  , {  1,   1,   32 }
  , {  4,   4,   64 }
  , {  8,   8,  128 }
  , { 16,  32,  128 }
  , { 32, 128,  256 }
  , { 64, 128, 1024 }
};

BlockConfig::config_array_type const BlockConfig::REDUCED_SET{
    {  1,  1,  1 }
  , {  1,  1, 32 }
  , {  4,  4, 64 }
  , {  8,  8, 64 }
};
