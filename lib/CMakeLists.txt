set(LIB_SRCS
  process.cpp)

add_library(lns-tools SHARED ${LIB_SRCS})
set_target_properties(lns-tools
  PROPERTIES VERSION 0.1
  SOVERSION 0)
install(TARGETS lns-tools DESTINATION "${CMAKE_INSTALL_LIBDIR}")
