# CxxCli
C++ Cli template library

Trivialises cli command parsing logic.

Tested on MSVC 19

## Usage
### Code
See tests

### CMake
- Add <CxxCli/modules> path to cmake module path
  - ```LIST(APPEND CMAKE_MODULE_PATH <CxxCli/modules>)```
- Include CxxCli module
  - ```include(IncludeCxxCli)```
- Link against ```CxxCliLib``` library
  - ```target_link_libraries(<library> CxxCliLib)```
- ```#include "CxxCli/CxxCli.hpp"```

## ToDo
+ source documentation
+ cmake module documentation
+ command usage
