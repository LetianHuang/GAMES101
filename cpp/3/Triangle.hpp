#pragma once

#include "Texture.hpp"
#include <algorithm>
#include <array>

#include <eigen3/Eigen/Eigen>

using namespace Eigen;

class Triangle
{
public:
    Vector4f v[3]; /*the original coordinates of the triangle, v0, v1, v2 in counter clockwise order*/
    /*Per vertex values*/
    Vector3f color[3];      // color at each vertex;
    Vector2f tex_coords[3]; // texture u,v
    Vector3f normal[3];     // normal vector for each vertex

    Texture *tex = nullptr;
    Triangle();

    Eigen::Vector4f a() const { return v[0]; }
    Eigen::Vector4f b() const { return v[1]; }
    Eigen::Vector4f c() const { return v[2]; }

    void setVertex(int ind, Vector4f ver);             /*set i-th vertex coordinates */
    void setNormal(int ind, Vector3f n);               /*set i-th vertex normal vector*/
    void setColor(int ind, float r, float g, float b); /*set i-th vertex color*/

    void setNormals(const std::array<Vector3f, 3> &normals);
    void setColors(const std::array<Vector3f, 3> &colors);
    void setTexCoord(int ind, Vector2f uv); /*set i-th vertex texture coordinate*/
    std::array<Vector4f, 3> toVector4() const;
};

Triangle::Triangle()
{
    v[0] << 0, 0, 0, 1;
    v[1] << 0, 0, 0, 1;
    v[2] << 0, 0, 0, 1;

    color[0] << 0.0, 0.0, 0.0;
    color[1] << 0.0, 0.0, 0.0;
    color[2] << 0.0, 0.0, 0.0;

    tex_coords[0] << 0.0, 0.0;
    tex_coords[1] << 0.0, 0.0;
    tex_coords[2] << 0.0, 0.0;
}

void Triangle::setVertex(int ind, Vector4f ver)
{
    v[ind] = ver;
}

void Triangle::setNormal(int ind, Vector3f n)
{
    normal[ind] = n;
}

void Triangle::setColor(int ind, float r, float g, float b)
{
    if ((r < 0.0) || (r > 255.) ||
        (g < 0.0) || (g > 255.) ||
        (b < 0.0) || (b > 255.))
    {
        fprintf(stderr, "ERROR! Invalid color values");
        fflush(stderr);
        exit(-1);
    }

    color[ind] = Vector3f((float)r / 255., (float)g / 255., (float)b / 255.);
    return;
}

void Triangle::setTexCoord(int ind, Vector2f uv)
{
    tex_coords[ind] = uv;
}

std::array<Vector4f, 3> Triangle::toVector4() const
{
    std::array<Vector4f, 3> res;
    std::transform(std::begin(v), std::end(v), res.begin(), [](auto &vec)
                   { return Vector4f(vec.x(), vec.y(), vec.z(), 1.f); });
    return res;
}

void Triangle::setNormals(const std::array<Vector3f, 3> &normals)
{
    normal[0] = normals[0];
    normal[1] = normals[1];
    normal[2] = normals[2];
}

void Triangle::setColors(const std::array<Vector3f, 3> &colors)
{
    auto first_color = colors[0];
    setColor(0, colors[0][0], colors[0][1], colors[0][2]);
    setColor(1, colors[1][0], colors[1][1], colors[1][2]);
    setColor(2, colors[2][0], colors[2][1], colors[2][2]);
}