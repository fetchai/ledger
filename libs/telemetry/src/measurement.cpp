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

#include "telemetry/measurement.hpp"

#include <ostream>

namespace fetch {
namespace telemetry {
namespace {

/**
 * Add the label information on to the stream if needed
 *
 * @param stream The stream to be populated
 * @param labels The labels of the measurement
 */
void WriteLabels(std::ostream &stream, Measurement::Labels const &labels)
{
  if (!labels.empty())
  {
    stream << '{';

    bool add_sep{false};
    for (auto const &element : labels)
    {
      // add the seperator if needed (all but the first loop)
      if (add_sep)
      {
        stream << ',';
      }

      // do the basic formatting
      stream << element.first << "=\"" << element.second << '"';

      // after the first element seperators should always be added
      add_sep = true;
    }

    stream << '}';
  }

  // add the value spacer
  stream << ' ';
}

}  // namespace

std::ostream &Measurement::WritePrefix(std::ostream &stream, char const *type_name) const
{
  stream << "# HELP " << name() << ' ' << description() << '\n'
         << "# TYPE " << name() << ' ' << type_name << '\n'
         << name();

  // add the labels
  WriteLabels(stream, labels_);

  return stream;
}

}  // namespace telemetry
}  // namespace fetch
