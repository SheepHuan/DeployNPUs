# DeployNPUs

## Run

```bash
git clone https://github.com/SheepHuan/DeployNPUs.git
git submodule update --init --recursive
```

## Support Platforms

1. RKNN2
    - RK3568 (2.2.0-2024-09-18), 支持op profiling. You can download SDK from [here](https://github.com/airockchip/rknn-toolkit2?tab=readme-ov-file#download).
2. HIAI
    - Kirin9000
3. BPU
    - SunriseX3
4. Hexogan
    - Snapdragon865

## Dependency
<!-- git submodule add https://github.com/google/glog.git 3rd-party/glog
git submodule add https://github.com/gflags/gflags.git 3rd-party/gflags -->
1. [gflags v2.2.2](https://github.com/gflags/gflags/tree/v2.2.2) 
2. [glog v0.7.1](https://github.com/google/glog/tree/v0.7.1)