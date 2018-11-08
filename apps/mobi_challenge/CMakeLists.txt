################################################################################
# F E T C H   M O B I   C H A L L E N G E   G E N E R A T O R
################################################################################
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(fetch-mobi_challenge-generator)

setup_compiler()

add_executable(mobi_challenge-generator
        main.cpp
        )
target_link_libraries(mobi_challenge-generator PRIVATE fetch-ledger fetch-core -bz2)

