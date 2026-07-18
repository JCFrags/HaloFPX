### Q: Can I use NPU for accelerate ML training?

A: You can use NPU to accelerate ML inference. But NPU is not designed for ML training.

### Q: How to allocate huge size BO?

A: There is no limit for BO size from the XRT and NPU device.
An application can fail to allocate a huge BO, once it hits the Linux resource limit.
In our test, the "max locked memory" is the key. You can follow below steps to check and change configure.
``` bash
ulimit -l # The result is in kbytes
