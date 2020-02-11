#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "core/string/ends_with.hpp"

#include <cstring>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <vector>

namespace fetch {
namespace ml {
namespace examples {

// TODO(?): also handle a string that specifies one text file only
/**
 * returns a vector of filenames of txt files
 * @param dir_name  the directory to scan
 * @return
 */
inline std::vector<std::string> GetAllTextFiles(std::string const &dir_name,
                                                std::string const &fileext = ".txt")
{
  std::vector<std::string> ret;
  DIR *                    d;
  struct dirent *          ent;
  std::string              p1;
  if ((d = opendir(dir_name.c_str())) != nullptr)
  {
    while ((ent = readdir(d)) != nullptr)
    {
      p1 = ent->d_name;
      if (fetch::core::EndsWith(p1, fileext))
      {
        ret.emplace_back(ent->d_name);
      }
    }
    closedir(d);
  }
  return ret;
}

/**
 * returns the full training text as one string, either gathered from txt files or passed through
 * @param training_data
 * @param full_training_text
 */
inline std::string GetTextString(std::string const &training_data)
{
  std::string              ret;
  std::vector<std::string> file_names = GetAllTextFiles(training_data);

  // no files at that location - assume the string is the training data directly
  if (file_names.empty())
  {
    ret = training_data;
  }

  // found the file at the location
  else
  {
    for (auto const &file_name : file_names)
    {
      std::string   cur_file = training_data + file_name;
      std::ifstream t(cur_file);

      std::string cur_text((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

      ret += cur_text;
      ret += ". ";
    }
  }
  return ret;
}

}  // namespace examples
}  // namespace ml
}  // namespace fetch
