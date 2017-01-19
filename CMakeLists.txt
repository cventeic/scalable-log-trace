cmake_minimum_required (VERSION 2.6)

project (scalable-log-trace)

set(CMAKE_BUILD_TYPE Debug)

set(WITH_NATIVE_NANOMSG 1)

if (WITH_NATIVE_NANOMSG)
  include_directories("." "../../common" )

  add_library(log_lib logger.c context.c discovery.c util.c fnv_hash_64a.c)
  target_link_libraries(log_lib LINK_PUBLIC nanomsg)

  add_executable(log_to_file log_to_file.c util.c base64.c)
  target_link_libraries(log_to_file LINK_PUBLIC nanomsg)

  add_executable(log_test_client log_test_client.c)
  target_link_libraries(log_test_client LINK_PUBLIC log_lib)
endif ()

set(WITH_VX_WORKS_NANOMSG 0)

if (WITH_VX_WORKS_NANOMSG)
  include_directories("." "../../common" ${CMAKE_BINARY_DIR}/../../nanomsg/build/pkg/include)
  link_directories(${CMAKE_BINARY_DIR}/../../nanomsg/build/pkg/lib)

  add_library(log_lib_vx logger.c context.c discovery.c util.c fnv_hash_64a.c)
  target_link_libraries(log_lib_vx LINK_PUBLIC nanomsg)

  add_executable(log_to_file_vx log_to_file.c util.c base64.c)
  target_link_libraries(log_to_file_vx LINK_PUBLIC nanomsg)

  add_executable(log_test_client_vx log_test_client.c)
  target_link_libraries(log_test_client_vx LINK_PUBLIC log_lib_vx)
endif ()

