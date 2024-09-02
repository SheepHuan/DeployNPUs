# Convert ONNX Model to RKNN Model


## 1. Install RKNN2 Toolkit

You should have download the RKNN2 Toolkit whl file and install it. The whl file is included in `sdk.tar.gz`. You can download sdk from [here](https://github.com/rockchip-linux/rknn-toolkit2?tab=readme-ov-file#download).

```bash
pip install sdks/2.0.0b23/packages/rknn_toolkit2-2.0.0b23+29ceb58d-cp310-cp310-linux_x86_64.whl
```

## 2. Convert

This repo contains a script `convert_rknn_model.py` to convert ONNX model to RKNN model. You can follow official document [here](https://github.com/airockchip/rknn_model_zoo/blob/main/examples/resnet/python/resnet.py), and try to convert ResNet to RKNN model.

```python
def convert_onnx_to_rknn(onnx_path,rknn_path,platform):
    from rknn.api import RKNN
    # Create RKNN object
    rknn = RKNN(verbose=True)

    # # Pre-process config
    # print('--> Config model')
    rknn.config(target_platform=platform)

    print('--> Loading model' + onnx_path)
    ret = rknn.load_onnx(model=onnx_path)
    if ret != 0:
        print(f'Load model {onnx_path} failed!')
        exit(ret)
    print('done')

    # Build model
    print('--> Building model')
    ret = rknn.build(do_quantization=False)
    if ret != 0:
        print(f'Build model {rknn_path} failed!')
        exit(ret)
    print('done')

    # Export rknn model
    print('--> Export rknn model')
    ret = rknn.export_rknn(rknn_path)
    if ret != 0:
        print('Export rknn model failed!')
        exit(ret)
    print('done')

    # Release
    rknn.release()

export_rknn(osp.join(onnx_dir,onnx_name),osp.join(rknn_dir,rknn_name),"rk3588")

```

## 3. Run

1. build
    ```bash
    mkdir build && cd build
    cmake .. -DBUILD_RKNN2=ON

    make -j
    ```

1. run single model
    ```bash
    ./rknn2_test --model=../saves/rknn2/MusicNN.rknn
    ```
    ![image.png](https://s2.loli.net/2024/08/03/LAhORkNE8TJ3Kl5.png)

2. run many models
    ```bash
    ./rknn2_test --model=../saves/rknn2/ --enable_batch_benchmark
    ```
    ![image.png](https://s2.loli.net/2024/08/03/n9ZywHgC1SMIqo5.png)
