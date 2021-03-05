# CxxCli
C++ Cli template library

Trivialises cli command parsing logic.

Tested on MSVC 19

## Usage
### Code
See tests
### CMake
- Add <CxxCli/find/FindCxxCliLib.cmake> module to cmake module path
  - ```LIST(APPEND CMAKE_MODULE_PATH <path containing FindCxxCliLib.cmake>)```
- Link against ```CxxCliLib``` library
  - ```target_link_libraries(<library> CxxCliLib)```
- #include "CxxCliLib/CxxCli.hpp"

## ToDo

+ source documentation
+ cmake module documentation
+ command usage
