option(BUILD_ARCHTEST "Build Arch Test" OFF)

if (BUILD_ARCHTEST)
    # 设置源文件目录
    set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/source/archtest)
    # message(STATUS " ${SOURCE_DIR}/bw_mem.c")
    # 检索.h和.cc文件
    # file(GLOB_RECURSE HPP_FILES "${SOURCE_DIR}/*.hpp")
    # file(GLOB_RECURSE H_FILES "${SOURCE_DIR}/*.h")
    # file(GLOB_RECURSE CC_FILES "${SOURCE_DIR}/*.cc")
    # file(GLOB_RECURSE C_FILES "${SOURCE_DIR}/*.c")

    include_directories(
        ${SOURCE_DIR}
    )

    add_executable(arch_test 
        ${SOURCE_DIR}/main.cc
    )
    set_target_properties(arch_test PROPERTIES LINKER_LANGUAGE CXX)

    target_link_libraries(arch_test 
                        PUBLIC
                        gflags::gflags 
                        glog::glog)
endif(BUILD_ARCHTEST)
