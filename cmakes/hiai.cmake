
option(BUILD_HIAI "Build HiAI" OFF)


set(HIAI_INCLUDE_DIR "/workspace/DeployNPUs/sdks/hwhiai-ddk-100.600.010.010/ddk/ai_ddk_lib/include")
set(HIAI_LIBRARY_DIR "/workspace/DeployNPUs/sdks/hwhiai-ddk-100.600.010.010/ddk/ai_ddk_lib/lib64")


if (BUILD_HIAI)
    add_library(HIAI INTERFACE)
    target_include_directories(HIAI INTERFACE ${HIAI_INCLUDE_DIR})
    target_link_libraries(HIAI INTERFACE 
                        ${HIAI_LIBRARY_DIR}/libhiai.so
                        
                        )
    add_executable(hiai_test ${CMAKE_SOURCE_DIR}/source/hiai/main.cpp)
    target_link_libraries(hiai_test PUBLIC HIAI gflags::gflags glog::glog)
endif()