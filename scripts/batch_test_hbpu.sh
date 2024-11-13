#!/bin/bash

# 指定要搜索的目录
search_dir="/home/sunrise/DeployNPUs/saves/onnx3-10"
benchmark_path="/home/sunrise/DeployNPUs/build/hbpu_test"
# 使用find命令查找所有.bin文件，并通过xargs传递给循环
find "$search_dir" -type f -name "*.bin" | while read -r file_path; do
    $benchmark_path --model "$file_path"
done