cmake_minimum_required(VERSION 3.10)

project(
  Headquarter
  VERSION 1.0
  LANGUAGES C
)

option(ENABLE_TESTING "Build tests" OFF)
option(HQ_ENABLE_TESTING "Build tests" OFF)

add_subdirectory(code)
add_subdirectory(deps)
add_subdirectory(plugins)

if (HQ_ENABLE_TESTING)
  add_subdirectory(tests)
endif()
