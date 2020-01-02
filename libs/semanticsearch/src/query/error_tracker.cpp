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

#include "semanticsearch/query/error_tracker.hpp"

#include <cassert>

namespace fetch {
namespace semanticsearch {

void ErrorTracker::Print()
{
  for (auto &err : errors_)
  {
    PrintErrorMessage(err);
    uint64_t endc = err.token().character() + err.token().size();
    PrintLine(err.token().line(), err.token().character(), endc);
  }
}

void ErrorTracker::RaiseSyntaxError(ConstByteArray message, Token token)
{
  errors_.emplace_back(filename_, source_, message, token, ErrorMessage::Type::SYNTAX_ERROR);
}

void ErrorTracker::RaiseRuntimeError(ConstByteArray message, Token token)
{
  errors_.emplace_back(filename_, source_, message, token, ErrorMessage::Type::RUNTIME_ERROR);
}

void ErrorTracker::RaiseInternalError(ConstByteArray message, Token token)
{
  errors_.emplace_back(filename_, source_, message, token, ErrorMessage::Type::INTERNAL_ERROR);
}

void ErrorTracker::SetSource(ConstByteArray source, ConstByteArray filename)
{
  source_   = std::move(source);
  filename_ = std::move(filename);
}

void ErrorTracker::ClearErrors()
{
  errors_.clear();
}

bool ErrorTracker::HasErrors() const
{
  return !errors_.empty();
}

void ErrorTracker::PrintLine(int line, uint64_t character, uint64_t char_end) const
{
  uint64_t position = 0;
  int      curline  = 0;

  if (char_end == uint64_t(-1))
  {
    char_end = character;
  }

  // Stopping one line before
  --line;
  while ((position < source_.size()) && (curline < line))
  {
    if (source_[position] == '\n')
    {
      ++curline;
    }
    ++position;
  }

  // Printing the line before
  std::cout << std::setw(4) << (line + 1) << ": | ";
  while ((position < source_.size()) && (curline == line))
  {
    if (source_[position] == '\n')
    {
      ++curline;
    }
    else
    {
      std::cout << source_[position];
    }
    ++position;
  }
  std::cout << std::endl;

  // Printing main line
  ++line;
  uint64_t char_count = 0;
  std::cout << fetch::commandline::VT100::Bold;
  std::cout << std::setw(4) << (line + 1) << fetch::commandline::VT100::DefaultAttributes();
  std::cout << ": | ";
  while ((position < source_.size()) && (curline == line))
  {
    if ((char_count == character) && (char_count < char_end))
    {
      std::cout << fetch::commandline::VT100::GetColor(3, 9);
    }
    else if (char_count == char_end)
    {
      std::cout << fetch::commandline::VT100::DefaultAttributes();
    }

    if (source_[position] == '\n')
    {
      ++curline;
    }
    else
    {
      std::cout << source_[position];
    }

    ++char_count;
    ++position;
  }
  std::cout << std::endl;

  // Printing one line after
  ++line;
  std::cout << std::setw(4) << (line + 1) << ": | ";
  while ((position < source_.size()) && (curline == line))
  {
    if (source_[position] == '\n')
    {
      ++curline;
    }
    else
    {
      std::cout << source_[position];
    }
    ++position;
  }
  std::cout << std::endl;
  std::cout << std::endl;
}

void ErrorTracker::PrintErrorMessage(ErrorMessage const &error)
{
  std::cout << fetch::commandline::VT100::Bold << error.filename() << ":" << (error.line() + 1)
            << ":" << error.character() << ": ";
  switch (error.type())
  {
  case ErrorMessage::Type::INFO:
    std::cout << fetch::commandline::VT100::GetColor(4, 9) << "info:";
    break;
  case ErrorMessage::Type::WARNING:
    std::cout << fetch::commandline::VT100::GetColor(3, 9) << "warning:";
    break;
  case ErrorMessage::Type::SYNTAX_ERROR:
  case ErrorMessage::Type::RUNTIME_ERROR:
    std::cout << fetch::commandline::VT100::GetColor(1, 9) << "error:";
    break;
  case ErrorMessage::Type::INTERNAL_ERROR:
    std::cout << fetch::commandline::VT100::GetColor(5, 9) << "internal error:";
    break;
  case ErrorMessage::Type::APPEND:
    break;
  }
  std::cout << fetch::commandline::VT100::DefaultAttributes() << " " << error.message()
            << std::endl;
  std::cout << std::endl;
}

}  // namespace semanticsearch
}  // namespace fetch
