
## Run
```bash
mkdir build/build_archtest_x86 && cd build/build_archtest_x86

cmake  -S .. \
-B build_archtest_x86 \
-DBUILD_ARCHTEST=ON

cmake --build build_archtest_x86 --parallel 12
```

```bash
export ANDROID_NDK="/workspace/dev/android-ndk-r27c"
cmake -S .. \
-B build_archtest_arm64_v8a \
-DBUILD_ARCHTEST=ON \
-DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
-DCMAKE_BUILD_TYPE=Release \
-DANDROID_ABI="arm64-v8a" \
-DANDROID_STL=c++_static \
-DANDROID_NATIVE_API_LEVEL=27

cmake --build build_archtest_arm64_v8a --parallel 12
```


```bash
./cache -c -M 16M -W 5 -N 10

taskset 0001 ./cache -c -M 32M -W 2 -N 20
# 7核心上运行
taskset 80 ./cache -c -M 8M -W 1 -N 1
taskset 20 ./cache -c -M 8M -W 2 -N 20
taskset 08 ./cache -c -M 8M -W 2 -N 20
taskset 02 ./cache -c -M 8M -W 2 -N 20
# huawei 
# L1 cache: 65536 bytes, 1.40 nanoseconds, 64 linesize, 8.02 parallelism
# L2 cache: 16777216 bytes, 4.52 nanoseconds, 256 linesize, 2.39 parallelism
# Memory latency: 24.53 nanoseconds, 1.92 parallelism
```

```bash
# 读取速度
./bw_mem -P 1 -W 1 -N 12 512000000 rd
# intel Size: 512.00 MB, Speed: 43658.85 MB/s
# huawei Size: 512.00 MB, Speed: 13783.82 MB/s
# 写入速度
./bw_mem -P 1 -W 1 -N 12 512000000 wr
# intel Size: 512.00 MB, Speed: 24019.34 MB/s
# huawei Size: 512.00 MB, Speed: 5669.99 MB/s
# 内存拷贝
./bw_mem -P 1 -W 1 -N 12 512000000 bcopy
# intel Size: 512.00 MB, Speed: 24209.18 MB/s
# huawei Size: 512.00 MB, Speed: 12114.33 MB/s
```