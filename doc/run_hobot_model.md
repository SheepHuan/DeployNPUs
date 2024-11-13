# Run Hobot Model

1. Follow official instructions [here](https://developer.horizon.cc/documents_rdk/linux_development/environment_build) to install GCC toolchain.
    ```bash
    curl -fO https://sunrise.horizon.cc/toolchain/gcc-ubuntu-9.3.0-2020.03-x86_64-aarch64-linux-gnu.tar.xz

    sudo tar -xvf gcc-ubuntu-9.3.0-2020.03-x86_64-aarch64-linux-gnu.tar.xz -C sdks/
    ```
2. Setup environment variables:
```bash
    export CROSS_COMPILE=/opt/gcc-ubuntu-9.3.0-2020.03-x86_64-aarch64-linux-gnu/bin/aarch64-linux-gnu-
    export LD_LIBRARY_PATH=/opt/gcc-ubuntu-9.3.0-2020.03-x86_64-aarch64-linux-gnu/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH
    export PATH=$PATH:/opt/gcc-ubuntu-9.3.0-2020.03-x86_64-aarch64-linux-gnu/bin/
    export ARCH=arm64

  ```