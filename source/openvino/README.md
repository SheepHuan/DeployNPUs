

## Run
```bash
mkdir build && cd build
cmake -S .. -DBUILD_OPENVINO=ON -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release 
cmake --build .  --config Release --parallel 12

$env:PATH += ";E:\workspace\code\openvino\build\install\runtime\bin\intel64\Release"



./openvino_test --model "D:\Downloads\deafault\ResNet50-opset12.xml" --bin "D:\Downloads\deafault\ResNet50-opset12.bin" --num_run 20

./openvino_test --model "D:\Downloads\deafault\MobileNetV2-opset12.xml" --bin "D:\Downloads\deafault\MobileNetV2-opset12.bin" --num_run 20

```


![image.png](https://s2.loli.net/2024/11/28/stOlhB2EeCwuoF9.png)