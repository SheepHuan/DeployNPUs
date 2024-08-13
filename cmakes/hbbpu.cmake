option(BUILD_HBBPU "Build bpu" OFF)
set(HBBPU_INCLUDE_DIR "/usr/include/dnn")
set(HBBPU_LIBRARY_DIR "/usr/lib/hbbpu")

if (BUILD_HBBPU)
    add_library(HBBPU INTERFACE)
    target_include_directories(HBBPU INTERFACE ${HBBPU_INCLUDE_DIR})
    target_link_libraries(HBBPU INTERFACE ${HBBPU_LIBRARY_DIR}/libdnn.so)
    
    # 设置源文件目录
    set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/source/bpu)

    # 检索.h和.cc文件
    file(GLOB_RECURSE H_FILES "${SOURCE_DIR}/*.h")
    file(GLOB_RECURSE CC_FILES "${SOURCE_DIR}/*.cc")

    add_executable(hbpu_test ${CC_FILES} ${H_FILES})
    target_link_libraries(hbpu_test 
                        PUBLIC 
                        HBBPU 
                        gflags::gflags 
                        glog::glog)
endif(BUILD_HBBPU)
