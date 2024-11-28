
option(BUILD_OPENVINO "Build OPENVINO" OFF)


# set(OPENVINO_INCLUDE_DIR "E:/workspace/code/openvino/build/install/runtime/include")
set(OPENVINO_RUNTIME_DIR "E:/workspace/code/openvino/build/install/runtime")


if (BUILD_OPENVINO)
    list(APPEND CMAKE_PREFIX_PATH ${OPENVINO_RUNTIME_DIR})
    find_package(OpenVINO REQUIRED COMPONENTS Runtime)
    message(STATUS "OpenVINO Runtime DIR: ${OPENVINO_RUNTIME_DIR}")
    message(STATUS "OpenVINO Found: ${OpenVINO_FOUND}")
    message(STATUS "OpenVINO Version: ${OpenVINO_VERSION}")
    # add_library(OPENVINO INTERFACE)
    # target_include_directories(OPENVINO INTERFACE ${OPENVINO_INCLUDE_DIR})
    include_directories(${OPENVINO_INCLUDE_DIR})

    add_executable(openvino_test ${CMAKE_SOURCE_DIR}/source/openvino/main.cc)
    target_link_libraries(openvino_test PUBLIC openvino::runtime gflags::gflags glog::glog)


    if(WIN32)
    add_custom_command(TARGET openvino_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${OPENVINO_RUNTIME_DIR}/bin/intel64/Release"
    $<TARGET_FILE_DIR:openvino_test>
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${OPENVINO_RUNTIME_DIR}/3rdparty/tbb/bin"  # 添加 TBB 依赖
    $<TARGET_FILE_DIR:openvino_test>
    )
    endif()
    

endif()