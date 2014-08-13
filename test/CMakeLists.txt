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

add_executable(test-mount
  test-mount.cpp)
target_link_libraries(test-mount
  lns-tools)

add_test(NAME test-mount COMMAND test-mount
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(test-exec
  test-exec.cpp)
target_link_libraries(test-exec
  lns-tools)

add_test(NAME test-exec COMMAND test-exec
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(test-nested
  test-nested.cpp)
target_link_libraries(test-nested
  lns-tools)

add_test(NAME test-nested COMMAND test-nested
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
