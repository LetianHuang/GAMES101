#pragma once

#include <eigen3/Eigen/Eigen>

typedef unsigned char u08;

#define MY_PI 3.1415926
#define TWO_PI (2.0 * MY_PI)

inline float deg2rad(float deg)
{
    return deg * MY_PI / 180;
}

inline float min_max(float a, float low=0.001f, float high=0.999f) 
{
    return a < low ? low : a > high ? high : a;
}