# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\baudix_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\baudix_autogen.dir\\ParseCache.txt"
  "baudix_autogen"
  )
endif()
