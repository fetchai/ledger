#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include <cstring>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <vector>

namespace fetch {
namespace ml {
namespace examples {

// TODO - also handle a string that specifies one text file only
/**
 * returns a vector of filenames of txt files
 * @param dir_name  the directory to scan
 * @return
 */
std::vector<std::string> GetAllTextFiles(std::string const &dir_name)
{
  std::vector<std::string> ret;
  DIR *                    d;
  struct dirent *          ent;
  char *                   p1;
  int                      txt_cmp;
  if ((d = opendir(dir_name.c_str())) != NULL)
  {
    while ((ent = readdir(d)) != NULL)
    {
      strtok(ent->d_name, ".");
      p1 = strtok(NULL, ".");
      if (p1 != NULL)
      {
        txt_cmp = std::strcmp(p1, "txt");
        if (txt_cmp == 0)
        {
          ret.emplace_back(ent->d_name);
        }
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
std::string GetTextString(std::string const &training_data)
{
  std::string              ret        = "";
  std::vector<std::string> file_names = GetAllTextFiles(training_data);

  // no files at that location - assume the string is the training data directly
  if (file_names.size() == 0)
  {
    ret = training_data;
  }

  // found the file at the location
  else
  {
    for (std::uint64_t j = 0; j < file_names.size(); ++j)
    {
      std::string   cur_file = training_data + "/" + file_names.at(j) + ".txt";
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
