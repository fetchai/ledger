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

#include <sstream>
#include <vector>

namespace fetch {
namespace settings {
namespace detail {

template <typename U>
void ToCommaSeparatedList(std::ostream &stream, std::vector<U> const &values)
{
  bool add_separator{false};
  for (auto const &value : values)
  {
    if (add_separator)
    {
      stream << ',';
    }

    stream << value;
    add_separator = true;
  }
}

template <typename U>
void FromCommaSeparatedList(std::istream &stream, std::vector<U> &values)
{
  // clear any existing values
  values.clear();

  // extract the string value
  std::string value{};
  stream >> value;

  if (!stream.fail())
  {
    // split the peers
    std::size_t position = 0;
    while (std::string::npos != position)
    {
      // locate the separator
      std::size_t const separator_position = value.find(',', position);

      // extract the sub-string
      std::string const sub_element = value.substr(position, (separator_position - position));

      // prepare the sub stream
      std::istringstream sub_stream{sub_element};

      // create the new value
      U element_value{};
      sub_stream >> element_value;

      // ensure that the stream was processed successfully
      if (sub_stream.fail())
      {
        break;
      }

      // add the new element to the list
      values.emplace_back(std::move(element_value));

      // update the position for the next search
      if (std::string::npos == separator_position)
      {
        position = std::string::npos;
      }
      else
      {
        position = separator_position + 1;  // advance past separator
      }
    }
  }
}

}  // namespace detail

template <typename U>
std::ostream &operator<<(std::ostream &stream, std::vector<U> const &value)
{
  detail::ToCommaSeparatedList(stream, value);
  return stream;
}

template <typename U>
std::istream &operator>>(std::istream &stream, std::vector<U> &value)
{
  detail::FromCommaSeparatedList(stream, value);
  return stream;
}

}  // namespace settings
}  // namespace fetch
