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

namespace fetch {
namespace ml {
namespace exceptions {

class NotImplemented : public std::runtime_error
{
public:
  explicit NotImplemented()
    : std::runtime_error("")
  {}

  explicit NotImplemented(std::string const &msg)
    : std::runtime_error(msg)
    , msg_(msg)
  {}

  const char *what() const noexcept override
  {
    if (!(msg_.empty()))
    {
      return msg_.c_str();
    }
    return "feature or operation mode not yet implemented";
  }

private:
  std::string msg_ = "";
};

class InvalidFile : public std::runtime_error
{
public:
  explicit InvalidFile()
    : std::runtime_error("")
  {}

  explicit InvalidFile(std::string const &msg)
    : std::runtime_error(msg)
    , msg_(msg)
  {}

  const char *what() const noexcept override
  {
    if (!(msg_.empty()))
    {
      return msg_.c_str();
    }
    return "cannot find file or filetype not valid";
  }

private:
  std::string msg_;
};

class InvalidMode : public std::runtime_error
{
public:
  explicit InvalidMode()
    : std::runtime_error("")
  {}

  explicit InvalidMode(std::string const &msg)
    : std::runtime_error(msg)
    , msg_(msg)
  {}

  const char *what() const noexcept override
  {
    if (!(msg_.empty()))
    {
      return msg_.c_str();
    }
    return "invalid mode selected";
  }

private:
  std::string msg_;
};

class InvalidInput : public std::runtime_error
{
public:
  explicit InvalidInput()
    : std::runtime_error("")
  {}

  explicit InvalidInput(std::string const &msg)
    : std::runtime_error(msg)
    , msg_(msg)
  {}

  const char *what() const noexcept override
  {
    if (!(msg_.empty()))
    {
      return msg_.c_str();
    }
    return "invalid input";
  }

private:
  std::string msg_;
};

class Timeout : public std::runtime_error
{
public:
  explicit Timeout()
    : std::runtime_error("")
  {}

  explicit Timeout(std::string const &msg)
    : std::runtime_error(msg)
    , msg_(msg)
  {}

  const char *what() const noexcept override
  {
    if (!(msg_.empty()))
    {
      return msg_.c_str();
    }
    return "error: feature timed out";
  }

private:
  std::string msg_;
};

}  // namespace exceptions
}  // namespace ml
}  // namespace fetch
