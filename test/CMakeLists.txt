add_executable(test-clone
  test-clone.cpp)
target_link_libraries(test-clone
  lns-tools)

add_test(NAME test-clone COMMAND test-clone)

add_executable(test-vm
  test-vm.cpp)
target_link_libraries(test-vm
  lns-tools)

add_test(NAME test-vm COMMAND test-vm)

add_executable(test-user
  test-user.cpp)
target_link_libraries(test-user
  lns-tools)

add_test(NAME test-user COMMAND test-user)
