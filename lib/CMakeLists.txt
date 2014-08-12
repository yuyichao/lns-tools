set(LIB_SRCS
  process.cpp)
set(LIB_HDRS
  process.h
  utils.h)

add_library(lns-tools SHARED ${LIB_SRCS})
set_target_properties(lns-tools
  PROPERTIES VERSION 0.1
  SOVERSION 0)
configure_file(lns-tools.pc.in lns-tools.pc @ONLY)

install(TARGETS lns-tools DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(FILES ${LIB_HDRS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/lns-tools.pc"
  DESTINATION "${PKG_CONFIG_PATH}")
