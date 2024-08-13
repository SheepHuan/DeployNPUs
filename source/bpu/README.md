
## Run
```bash
mkdir build && cd build
cmake .. -DBUILD_HBBPU=ON
make -j

./hbpu_test --model /home/sunrise/DeployNPUs/saves/onnx-10/YoLoV3-opset10_output/YoLoV3-opset10_prefix.bin

```