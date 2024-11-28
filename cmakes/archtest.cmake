option(BUILD_ARCHTEST "Build Arch Test" OFF)

if (BUILD_ARCHTEST)
    # 设置源文件目录
    set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/source/archtest)

    include_directories(
        ${SOURCE_DIR}
        ${SOURCE_DIR}/lmbench
    )

    add_executable(arch_test 
        ${SOURCE_DIR}/main.cc
    )
    set_target_properties(arch_test PROPERTIES LINKER_LANGUAGE CXX)

    target_link_libraries(arch_test 
                        PUBLIC
                        gflags::gflags 
                        glog::glog)

    add_executable(
        cache
        ${SOURCE_DIR}/lmbench/cache.c
        ${SOURCE_DIR}/lmbench/getopt.c
        ${SOURCE_DIR}/lmbench/lib_debug.c
        ${SOURCE_DIR}/lmbench/lib_mem.c
        ${SOURCE_DIR}/lmbench/lib_timing.c
        ${SOURCE_DIR}/lmbench/lib_sched.c
    )
    set_target_properties(cache PROPERTIES LINKER_LANGUAGE C)

    add_executable(
        bw_mem
        ${SOURCE_DIR}/lmbench/bw_mem.c
        ${SOURCE_DIR}/lmbench/getopt.c
        ${SOURCE_DIR}/lmbench/lib_debug.c
        ${SOURCE_DIR}/lmbench/lib_mem.c
        ${SOURCE_DIR}/lmbench/lib_timing.c
        ${SOURCE_DIR}/lmbench/lib_sched.c
    )
    set_target_properties(bw_mem PROPERTIES LINKER_LANGUAGE C)


    set_source_files_properties(
        ${SOURCE_DIR}/lmbench/cache.c
        ${SOURCE_DIR}/lmbench/bw_mem.c
        ${SOURCE_DIR}/lmbench/getopt.c
        ${SOURCE_DIR}/lmbench/lib_debug.c
        ${SOURCE_DIR}/lmbench/lib_mem.c
        ${SOURCE_DIR}/lmbench/lib_timing.c
        ${SOURCE_DIR}/lmbench/lib_sched.c
        PROPERTIES LANGUAGE C
    )
    target_link_libraries(cache 
                        PUBLIC 
                        m)
    target_link_libraries(bw_mem 
                        PUBLIC 
                        m)
    
endif(BUILD_ARCHTEST)
