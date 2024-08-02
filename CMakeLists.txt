# 指定CMake的最低版本要求
cmake_minimum_required(VERSION 3.18)

# 项目名称和语言
project(DeployNPUs VERSION 1.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(
    ${CMAKE_SOURCE_DIR}/source/include
)

include(cmakes/dependency.cmake)
include(cmakes/rknn2.cmake)
# # 添加一个可执行文件
# add_executable(MyExecutable main.cpp)

# # 添加需要的头文件
# target_include_directories(MyExecutable PRIVATE include)

# # 添加一个库
# add_library(MyLibrary STATIC src/library.cpp)

# # 将库链接到可执行文件
# target_link_libraries(MyExecutable PRIVATE MyLibrary)