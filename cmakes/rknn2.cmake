
option(BUILD_RKNN2 "Build rknn2" OFF)


set(RKNN2_INCLUDE_DIR "/home/orangepi/code/DeployNPUs/sdks/2.0.0b23/runtime/Linux/librknn_api/include")

set(RKNN2_LIBRARY_DIR "/home/orangepi/code/DeployNPUs/sdks/2.0.0b23/runtime/Linux/librknn_api/aarch64")


if (BUILD_RKNN2)
    add_library(RKNN2 INTERFACE)
    target_include_directories(RKNN2 INTERFACE ${RKNN2_INCLUDE_DIR})
    target_link_libraries(RKNN2 INTERFACE ${RKNN2_LIBRARY_DIR}/librknnrt.so)
    add_executable(rknn2_test ${CMAKE_SOURCE_DIR}/source/rknn2/main.cpp)
    target_link_libraries(rknn2_test PUBLIC RKNN2 gflags::gflags glog::glog)
endif()