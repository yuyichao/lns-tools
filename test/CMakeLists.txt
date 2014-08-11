add_executable(test-clone
  test-clone.cpp)
target_link_libraries(test-clone
  lns-tools)

add_test(COMMAND test-clone)

add_executable(test-vm
  test-vm.cpp)
target_link_libraries(test-vm
  lns-tools)

add_test(COMMAND test-vm)
