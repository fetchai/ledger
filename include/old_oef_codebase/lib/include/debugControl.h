#pragma once

#include <exception>

// Temporary functions to print program state
void printError(const char *file, int line, const char* message);
void printMessage(const char *file, int line, const char* message);

template <typename T>
void printError(const char *file, int line, T &message)
{
  printError(file, line, message.c_str());
}

template <typename T>
void printMessage(const char *file, int line, T &message)
{
  printMessage(file, line, message.c_str());
}

// Define exception to use when things go wrong
class protocolException: public std::exception
{
 public:
  protocolException(const char *message) :
    _message{message}
  { }

  virtual const char* what() const throw()
  {
    return _message.c_str();
  }

  std::string _message;
};
