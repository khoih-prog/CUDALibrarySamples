# cuSPARSE Generic APIs - `cusparseSpMV SELL`

## Description

This sample demonstrates the usage of `cusparseSpMV` for performing *sparse matrix - dense vector multiplication*, where the sparse matrix is represented in SELL (Sliced Ellpack) storage format.

[cusparseSpMV Documentation](https://docs.nvidia.com/cuda/cusparse/index.html#cusparse-generic-function-spmv)

<center>

`Y = alpha * A * X + beta * Y`

![](spmv_sell.png)
</center>

## Building

* Command line
    ```bash
    nvcc -I<cuda_toolkit_path>/include spmv_sell_example.c -o spmv_sell_example -lcusparse
    ```

* Linux
    ```bash
    make
    ```

* Windows/Linux
    ```bash
    mkdir build
    cd build
    cmake ..
    make
    ```
    On Windows, instead of running the last build step, open the Visual Studio Solution that was created and build.

## Support

* **Supported SM Architectures:** SM 5.0, SM 5.2, SM 5.3, SM 6.0, SM 6.1, SM 6.2, SM 7.0, SM 7.2, SM 7.5, SM 8.0, SM 8.6, SM 8.9, SM 9.0
* **Supported OSes:** Linux, Windows, QNX, Android
* **Supported CPU Architectures**: x86_64, arm64
* **Supported Compilers**: gcc, clang, Intel icc, Microsoft msvc, Nvidia HPC SDK nvc
* **Language**: `C99`

## Prerequisites

* [CUDA 12.2 toolkit](https://developer.nvidia.com/cuda-downloads) (or above) and compatible driver (see [CUDA Driver Release Notes](https://docs.nvidia.com/cuda/cuda-toolkit-release-notes/index.html#cuda-major-component-versions)).
* [CMake 3.9](https://cmake.org/download/) or above on Windows
