project(lns-tools)

## Setup
cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(LNS_VERSION 0.1)

include(CMakeCMacros)
include(GNUInstallDirs)

cmake_c_fix_include_path(lib lns_tools)
cmake_c_include_fix_path()

cmake_c_add_flags(CMAKE_C_FLAGS -Wall -Wextra -std=gnu99 -fvisibility=hidden)
# -std=c++0x is deprecated but gcc < 4.7 do not recognise c++11 ....
cmake_c_add_flags(CMAKE_CXX_FLAGS -Wall -Wextra
  -Wno-deprecated-register -std=c++0x -fvisibility=hidden)
cmake_c_add_flags(CMAKE_SHARED_LINKER_FLAGS -Wl,--as-needed -Wl,--no-undefined)
cmake_c_add_flags(CMAKE_MODULE_LINKER_FLAGS -Wl,--as-needed -Wl,--no-undefined)

enable_testing()

if(NOT DEFINED PKG_CONFIG_PATH)
  set(PKG_CONFIG_PATH "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
endif()

add_subdirectory(lib)
add_subdirectory(test)
add_subdirectory(bin)
