# GAMES101项目

## GAMES101作业说明

GAMES101作业，仅供参考！

cpp目录中包含cpp源码以及用到的资源（如obj等），目前更新到cpp版本HW4，即贝塞尔曲线。

其中代码文件稍加改动，不影响结果以及核心思路。

1.主要改动包括将hpp对应的cpp文件移动到了hpp中且删除了cpp文件。<p>
2.hpp文件中```#pragma once```的预处理指令代替了```#ifndef``` <p>
3.其次作业HW1中实现了提高要求，即对过原点任意轴进行旋转的变换矩阵。<p>
4.作业HW2中实现了提高要求，且对rasterizer进行了部分修改，使之能够实现super-sampling进行抗锯齿。<p>
5.作业HW4中实现了提高要求，具体对应shading函数，当$anti\ne{0}$时，使用了反走样。<p>

cpp_out目录中包含cpp代码的部分运行结果图像。

pdf目录中包含GAMES官网中给定的作业题目。

## 环境与配置

OS系统：windows10

cpp：mingw c++17

依赖：OpenCV4.1.2_contrib_MinGW64-master, Eigen3
