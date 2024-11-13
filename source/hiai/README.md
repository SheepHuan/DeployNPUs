```bash 
export ANDROID_NDK="/workspace/dev/android-ndk-r27c"

cmake -S .. \
-B build_hiai_arm64_v8a \
-DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
-DCMAKE_BUILD_TYPE=Release \
-DANDROID_ABI="arm64-v8a" \
-DANDROID_STL=c++_static \
-DANDROID_NATIVE_API_LEVEL=27 \
-DBUILD_HIAI=ON
```