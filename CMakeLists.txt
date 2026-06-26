# cmake build so the project compiles the same way on linux, windows and mac.
# the makefile is still there for a quick local `make`, this is mostly for CI.
cmake_minimum_required(VERSION 3.16)
project(black_scholes_option_pricing CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# default to a release build if nobody said otherwise, no point shipping -O0
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(option_pricing main.cpp black_scholes.cpp)

# turn warnings on. msvc speaks a totaly different flag dialect to gcc/clang
if(MSVC)
  target_compile_options(option_pricing PRIVATE /W4)
else()
  target_compile_options(option_pricing PRIVATE -Wall -Wextra)
endif()

# wire the built in self checks into ctest so CI can just run `ctest`.
# the program returns a nonzero exit code if any check fails, which is exactly
# what ctest treats as a FAIL
enable_testing()
add_test(NAME selftest COMMAND option_pricing selftest)

# so `cmake --install` drops the binary somewhere predictable for the release step
install(TARGETS option_pricing RUNTIME DESTINATION bin)
