add_executable(bind_dirs
  bind_dirs.cpp)
target_link_libraries(bind_dirs
  lns-tools)

install(TARGETS bind_dirs DESTINATION "${CMAKE_INSTALL_BINDIR}")
