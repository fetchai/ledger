#
# F E T C H   A U C T I O N S   T E S T S
#
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(fetch-auctions)

# CMake configuration
include(${FETCH_ROOT_CMAKE_DIR}/BuildTools.cmake)

# Compiler Configuration
setup_compiler()

fetch_add_test(first_price_auction_gtest fetch-auctions auctions/first_price_auction)
fetch_add_test(vickrey_auction_gtest fetch-auctions auctions/vickrey_auction)
fetch_add_test(combinatorial_auction_gtest fetch-auctions auctions/combinatorial_auction)
