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

#include <stdexcept>
#include <string>

namespace fetch {
namespace math {
namespace exceptions {

class WrongIndices : public std::runtime_error
{
public:
  explicit WrongIndices()
    : std::runtime_error("")
  {}

  explicit WrongIndices(std::string const &msg)
    : std::runtime_error(msg)
    , msg_(msg)
  {}

  const char *what() const noexcept override
  {
    if (!(msg_.empty()))
    {
      return msg_.c_str();
    }
    return "Tensor invoked with wrong number of indices";
  }

private:
  std::string msg_;
};

class WrongShape : public std::runtime_error
{
public:
  explicit WrongShape()
    : std::runtime_error("")
  {}

  explicit WrongShape(std::string const &msg)
    : std::runtime_error(msg)
    , msg_(msg)
  {}

  const char *what() const noexcept override
  {
    if (!(msg_.empty()))
    {
      return msg_.c_str();
    }
    return "math operation invoked with wrong shape inputs";
  }

private:
  std::string msg_;
};

class NegativeLog : public std::runtime_error
{
public:
  explicit NegativeLog()
    : std::runtime_error("")
  {}

  explicit NegativeLog(std::string const &msg)
    : std::runtime_error(msg)
  {}

  const char *what() const noexcept override
  {
    if (!(msg_.empty()))
    {
      return msg_.c_str();
    }
    return "math operation attempted to take log of negative value which is undefined";
  }

private:
  std::string msg_;
};

class InvalidReshape : public std::runtime_error
{
public:
  explicit InvalidReshape()
    : std::runtime_error("")
  {}

  explicit InvalidReshape(std::string const &msg)
    : std::runtime_error(msg)
    , msg_(msg)
  {}

  const char *what() const noexcept override
  {
    if (!(msg_.empty()))
    {
      return msg_.c_str();
    }
    return "Not possible to perform requested reshape";
  }

private:
  std::string msg_;
};

class InvalidNumericCharacter : public std::runtime_error
{
public:
  explicit InvalidNumericCharacter()
    : std::runtime_error("")
  {}

  explicit InvalidNumericCharacter(std::string const &msg)
    : std::runtime_error(msg)
  {}

  const char *what() const noexcept override
  {
    return "attempted to assign data to tensor using invalid character";
  }
};

class InvalidMode : public std::runtime_error
{
public:
  explicit InvalidMode()
    : std::runtime_error("")
  {}

  explicit InvalidMode(std::string const &msg)
    : std::runtime_error(msg)
  {}

  const char *what() const noexcept override
  {
    return "invalid mode selected";
  }
};

}  // namespace exceptions
}  // namespace math
}  // namespace fetch
