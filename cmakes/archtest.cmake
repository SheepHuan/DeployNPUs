option(BUILD_ARCHTEST "Build Arch Test" OFF)

if (BUILD_ARCHTEST)
    # 设置源文件目录
    set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/source/archtest)

    # 检索.h和.cc文件
    file(GLOB_RECURSE H_FILES "${SOURCE_DIR}/*.hpp")
    file(GLOB_RECURSE CC_FILES "${SOURCE_DIR}/*.cc")

    add_executable(arch_test ${CC_FILES} ${H_FILES})
    target_link_libraries(arch_test 
                        PUBLIC 
                        gflags::gflags 
                        glog::glog)
endif(BUILD_ARCHTEST)
