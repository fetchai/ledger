# Note: when executed in the build dir, then CMAKE_CURRENT_SOURCE_DIR is the
# build dir.
file( COPY libai DESTINATION "${CMAKE_ARGV3}"
  FILES_MATCHING PATTERN "*.py" )

file( COPY libai/data DESTINATION "${CMAKE_ARGV3}/libai"
  FILES_MATCHING PATTERN "*" )
