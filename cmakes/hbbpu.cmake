option(BUILD_HBBPU "Build bpu" OFF)
set(HBBPU_INCLUDE_DIR "/usr/include/dnn")
set(HBBPU_LIBRARY_DIR "/usr/lib/hbbpu")

if (BUILD_HBBPU)
    add_library(HBBPU INTERFACE)
    target_include_directories(HBBPU INTERFACE ${HBBPU_INCLUDE_DIR})
    target_link_libraries(HBBPU INTERFACE ${HBBPU_LIBRARY_DIR}/libdnn.so)
    add_executable(hbbpu_test ${CMAKE_SOURCE_DIR}/source/bpu/main.cpp ${CMAKE_SOURCE_DIR}/source/bpu/MyBPU.hpp)
    target_link_libraries(hbbpu_test PUBLIC HBBPU gflags::gflags glog::glog)
endif(BUILD_HBBPU)
