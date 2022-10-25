# GAMES101项目

## GAMES101作业说明

GAMES101作业，仅供参考！

cpp目录中包含cpp源码以及用到的资源（如obj等），目前更新到cpp版本HW7，即光线追踪——路径追踪。

其中代码文件稍加改动，不影响结果以及核心思路。

1. 主要改动包括将hpp对应的cpp文件移动到了hpp中且删除了cpp文件。
2. hpp文件中```#pragma once```的预处理指令代替了```#ifndef```。
3. 其次作业HW1中实现了提高要求，即对过原点任意轴进行旋转的变换矩阵。
4. 作业HW2中实现了提高要求，且对rasterizer进行了部分修改，使之能够实现super-sampling进行抗锯齿。
5. 作业HW4中实现了提高要求，具体对应shading函数，当 $anti\ne{0}$ 时，使用了反走样。
6. 作业HW7中使用了openmp进行cpu并行计算，编译命令需要增加```-fopenmp```；且采样了MSAA抗锯齿；同时实现了Microfacet材质。

cpp_out目录中包含cpp代码的部分运行结果图像。

pdf目录中包含GAMES官网中给定的作业题目。

## 环境与配置

OS系统：windows10

cpp：mingw c++17

依赖：OpenCV4.1.2_contrib_MinGW64-master, Eigen3

## 项目部分图像展示（详情见cpp_out目录）

### Transformation

<img src="https://github.com/mofashaoye/GAMES101/blob/main/cpp_out/1/output.png" />

### Rasterization and Z-Buffer

<img src="https://github.com/mofashaoye/GAMES101/blob/main/cpp_out/2/output.png" />

### Shading

#### normal

<img src="https://github.com/mofashaoye/GAMES101/blob/main/cpp_out/3/normal.png" />

#### phong

<img src="https://github.com/mofashaoye/GAMES101/blob/main/cpp_out/3/phong.png" />

#### texture

<img src="https://github.com/mofashaoye/GAMES101/blob/main/cpp_out/3/texture.png" />

#### bump

<img src="https://github.com/mofashaoye/GAMES101/blob/main/cpp_out/3/bump.png" />

#### displacement

<img src="https://github.com/mofashaoye/GAMES101/blob/main/cpp_out/3/displacement.png" />

### Bézier Curve

<img src="https://github.com/mofashaoye/GAMES101/blob/main/cpp_out/4/my_bezier_curve.png" />

### Ray Tracing——Ray triangle intersection

<img src="https://github.com/mofashaoye/GAMES101/blob/main/cpp_out/5/binary.png" />

### Ray Tracing——Bounding Volume Hierarchy (BVH)

<img src="https://github.com/mofashaoye/GAMES101/blob/main/cpp_out/6/binary.png" />

### Ray Tracing——Path Tracing

#### Scene1 spp1024 784x784 threads12

<img src="https://github.com/mofashaoye/GAMES101/blob/main/cpp_out/7/scene1_784x784_spp1024_numw12.png" />

#### Scene1 spp1024 784x784 threads12 MSAA

<img src="https://github.com/mofashaoye/GAMES101/blob/main/cpp_out/7/scene1_784x784_spp1024_numw12_msaa.png" />

#### Scene1 spp1024 784x784 threads12 MSAA gamma

<img src="https://github.com/mofashaoye/GAMES101/blob/main/cpp_out/7/scene1_784x784_spp1024_numw12_msaa_gamma.png" />

#### Scene2 spp1024 784x784 threads12 MSAA

<img src="https://github.com/mofashaoye/GAMES101/blob/main/cpp_out/7/scene2_784x784_spp1024_numw12_msaa.png" />

#### Scene2 spp1024 784x784 threads12 MSAA gamma

<img src="https://github.com/mofashaoye/GAMES101/blob/main/cpp_out/7/scene2_784x784_spp1024_numw12_msaa_gamma.png" />


#### Scene3 spp1024 784x784 threads12 MSAA

<img src="https://github.com/mofashaoye/GAMES101/blob/main/cpp_out/7/scene3_784x784_spp1024_numw12_msaa.png" />

#### Scene3 spp1024 784x784 threads12 MSAA gamma

<img src="https://github.com/mofashaoye/GAMES101/blob/main/cpp_out/7/scene3_784x784_spp1024_numw12_msaa_gamma.png" />
