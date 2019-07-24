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

#include <cassert>
#include <ostream>

namespace fetch {
namespace telemetry {
namespace {

struct LabelRefs
{
  Measurement::Labels const *main{nullptr};
  Measurement::Labels const *extra{nullptr};

  explicit LabelRefs(Measurement::Labels const &main)
    : main{&main}
  {}

  LabelRefs(Measurement::Labels const &main, Measurement::Labels const &other)
    : main{&main}
    , extra{&other}
  {}
};

std::ostream &operator<<(std::ostream &stream, LabelRefs const &refs)
{
  assert(refs.main);

  // check to see if any of the labels exist
  if ((refs.main && !refs.main->empty()) || (refs.extra && !refs.extra->empty()))
  {
    stream << '{';

    bool add_sep{false};
    for (auto const *container : {refs.main, refs.extra})
    {
      if (!container)
      {
        continue;
      }

      for (auto const &element : *container)
      {
        // add the separator if needed (all but the first loop)
        if (add_sep)
        {
          stream << ',';
        }

        // do the basic formatting
        stream << element.first << "=\"" << element.second << '"';

        // after the first element separators should always be added
        add_sep = true;
      }
    }

    stream << '}';
  }

  // add the value spacer
  stream << ' ';
  return stream;
}

}  // namespace

OutputStream::OutputStream(std::ostream &stream)
  : stream_{stream}
{
}

bool OutputStream::HeaderIsRequired(std::string const &name)
{
  auto const result = metrics_.emplace(name);
  return result.second;
}

OutputStream &Measurement::WriteHeader(OutputStream &stream, char const *type_name) const
{
  if (stream.HeaderIsRequired(name()))
  {
    stream << "# HELP " << name() << ' ' << description() << '\n'
           << "# TYPE " << name() << ' ' << type_name << '\n';
  }

  return stream;
}

OutputStream &Measurement::WriteValuePrefix(OutputStream &stream) const
{
  stream << name() << LabelRefs{labels_};
  return stream;
}

OutputStream &Measurement::WriteValuePrefix(OutputStream &stream, std::string const &suffix) const
{
  stream << name() << '_' << suffix << LabelRefs{labels_};
  return stream;
}

OutputStream &Measurement::WriteValuePrefix(OutputStream &stream, std::string const &suffix,
                                            Labels const &extra) const
{
  stream << name() << '_' << suffix << LabelRefs{labels_, extra};
  return stream;
}

}  // namespace telemetry
}  // namespace fetch
