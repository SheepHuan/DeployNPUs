

## Run
```bash
mkdir build && cd build
cmake -S .. -DBUILD_OPENVINO=ON -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release 
cmake --build .  --config Release --parallel 12

$env:PATH += ";E:\workspace\code\openvino\build\install\runtime\bin\intel64\Release"



./openvino_test --model "D:\Downloads\deafault\openvino" --num_run 10 --num_warmup 5

./openvino_test --model "D:\Downloads\deafault\openvino-2" --num_run 1 --num_warmup 1

./openvino_test --model "D:\Downloads\deafault\openvino\YoLoV3-opset12.xml" --num_run 1 --num_warmup 5
./openvino_test --model "D:\Downloads\deafault\openvino\Swin-opset12.xml"  --num_run 20 --num_warmup 5
./openvino_test --model "D:\Downloads\deafault\openvino\DenseNet-opset12.xml"  --num_run 20 --num_warmup 5
./openvino_test --model "D:\Downloads\deafault\openvino\EfficientNet-opset12.xml"  --num_run 20 --num_warmup 5
./openvino_test --model "D:\Downloads\deafault\openvino\GhostNet-opset12.xml"  --num_run 20 --num_warmup 5
./openvino_test --model "D:\Downloads\deafault\openvino\InceptionV3-opset12.xml"  --num_run 20 --num_warmup 5
./openvino_test --model "D:\Downloads\deafault\openvino\MicroNet-opset12.xml" --num_run 20 --num_warmup 5
./openvino_test --model "D:\Downloads\deafault\openvino\MNASNet-opset12.xml" --num_run 20 --num_warmup 5
./openvino_test --model "D:\Downloads\deafault\openvino\MobileNetV2-opset12.xml" --num_run 20 --num_warmup 5
./openvino_test --model "D:\Downloads\deafault\openvino\MusicNN-opset12.xml" --num_run 20 --num_warmup 5
./openvino_test --model "D:\Downloads\deafault\openvino\OnePose-opset12.xml" --num_run 20 --num_warmup 5
./openvino_test --model "D:\Downloads\deafault\openvino\RegNet-opset12.xml" --num_run 20 --num_warmup 5
./openvino_test --model "D:\Downloads\deafault\openvino\ResNet50-opset12.xml" --num_run 20 --num_warmup 5
./openvino_test --model "D:\Downloads\deafault\openvino\ShuffleNetV2-opset12.xml" --num_run 20 --num_warmup 5
./openvino_test --model "D:\Downloads\deafault\openvino\SqueezeNet-opset12.xml" --num_run 20 --num_warmup 5
./openvino_test --model "D:\Downloads\deafault\openvino\SRCNN-opset12.xml" --num_run 20 --num_warmup 5
./openvino_test --model "D:\Downloads\deafault\openvino\SSD-opset12.xml" --num_run 20 --num_warmup 5
./openvino_test --model "D:\Downloads\deafault\openvino\UNet-opset12.xml" --num_run 20 --num_warmup 5
./openvino_test --model "D:\Downloads\deafault\openvino\VGG11-opset12.xml" --num_run 20 --num_warmup 5
./openvino_test --model "D:\Downloads\deafault\openvino\WaveLetter-opset12.xml" --num_run 20 --num_warmup 5

```


![image.png](https://s2.loli.net/2024/11/28/stOlhB2EeCwuoF9.png)