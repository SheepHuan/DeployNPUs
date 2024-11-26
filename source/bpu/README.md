
## Run
```bash
mkdir build && cd build
cmake .. -DBUILD_HBBPU=ON
make -j

./hbpu_test \
--enable_batch_benchmark true \
--model /home/sunrise/DeployNPUs/saves/bins \
--num_warmup 3 \
--num_run 10

```