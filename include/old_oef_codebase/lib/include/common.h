// Company: FETCH.ai
// Author: Jerome Maloberti
// Creation: 09/01/18
//
// Common functions used when making network connections

#pragma once

#include <string>
#include <functional>

#include "asio.hpp"

bool blockedWrite(asio::ip::tcp::socket &socket, const std::string &s);
void asyncWrite(asio::ip::tcp::socket &socket, const std::string &s);
void asyncRead(asio::ip::tcp::socket &socket, std::function<void(std::error_code,std::string)> handler);
