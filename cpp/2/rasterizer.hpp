#pragma once

#include "global.hpp"
#include "Triangle.hpp"
#include "rasterizer.hpp"

#include <algorithm>
#include <vector>
#include <math.h>

#include <eigen3/Eigen/Eigen>
#include <opencv2/opencv.hpp>

using namespace Eigen;

namespace rst
{
    enum class Buffers
    {
        Color = 1,
        Depth = 2
    };

    inline Buffers operator|(Buffers a, Buffers b)
    {
        return Buffers((int)a | (int)b);
    }

    inline Buffers operator&(Buffers a, Buffers b)
    {
        return Buffers((int)a & (int)b);
    }

    enum class Primitive
    {
        Line,
        Triangle
    };

    /*
     * For the curious : The draw function takes two buffer id's as its arguments. These two structs
     * make sure that if you mix up with their orders, the compiler won't compile it.
     * Aka : Type safety
     * */
    struct pos_buf_id
    {
        int pos_id = 0;
    };

    struct ind_buf_id
    {
        int ind_id = 0;
    };

    struct col_buf_id
    {
        int col_id = 0;
    };

    class rasterizer
    {
    public:
        std::set<std::tuple<float, float, float>> col_st;

        rasterizer(int w, int h, int ss);
        pos_buf_id load_positions(const std::vector<Eigen::Vector3f> &positions);
        ind_buf_id load_indices(const std::vector<Eigen::Vector3i> &indices);
        col_buf_id load_colors(const std::vector<Eigen::Vector3f> &colors);

        void set_model(const Eigen::Matrix4f &m);
        void set_view(const Eigen::Matrix4f &v);
        void set_projection(const Eigen::Matrix4f &p);

        void set_pixel(const Eigen::Vector3f &point, const Eigen::Vector3f &color);

        void clear(Buffers buff);

        void draw(pos_buf_id pos_buffer, ind_buf_id ind_buffer, col_buf_id col_buffer, Primitive type);

        std::vector<Eigen::Vector3f> &frame_buffer() { return frame_buf; }

    private:
        void draw_line(Eigen::Vector3f begin, Eigen::Vector3f end);

        void rasterize_triangle(const Triangle &t);

        // VERTEX SHADER -> MVP -> Clipping -> /.W -> VIEWPORT -> DRAWLINE/DRAWTRI -> FRAGSHADER

    private:
        Eigen::Matrix4f model;
        Eigen::Matrix4f view;
        Eigen::Matrix4f projection;

        std::map<int, std::vector<Eigen::Vector3f>> pos_buf;
        std::map<int, std::vector<Eigen::Vector3i>> ind_buf;
        std::map<int, std::vector<Eigen::Vector3f>> col_buf;

        std::vector<Eigen::Vector3f> frame_buf;

        std::vector<float> depth_buf;
        std::vector<Eigen::Vector3f> color_buf;
        int get_index(int x, int y);

        int width, height;
        
        int super_sample;

        int next_id = 0;
        int get_next_id() { return next_id++; }
    };

    rst::pos_buf_id rst::rasterizer::load_positions(const std::vector<Eigen::Vector3f> &positions)
    {
        auto id = get_next_id();
        pos_buf.emplace(id, positions);

        return {id};
    }

    rst::ind_buf_id rst::rasterizer::load_indices(const std::vector<Eigen::Vector3i> &indices)
    {
        auto id = get_next_id();
        ind_buf.emplace(id, indices);

        return {id};
    }

    rst::col_buf_id rst::rasterizer::load_colors(const std::vector<Eigen::Vector3f> &cols)
    {
        auto id = get_next_id();
        col_buf.emplace(id, cols);

        return {id};
    }

    auto to_vec4(const Eigen::Vector3f &v3, float w = 1.0f)
    {
        return Vector4f(v3.x(), v3.y(), v3.z(), w);
    }

    static bool insideTriangle(float x, float y, const Vector3f *_v)
    {
        // TODO : Implement this function to check if the point (x, y) is inside the triangle represented by _v[0], _v[1], _v[2]
        Vector3f v[3];
        for(int i=0;i<3;i++)
            v[i] = {_v[i].x(),_v[i].y(), 1.0};
        Vector3f f0,f1,f2;
        f0 = v[1].cross(v[0]);
        f1 = v[2].cross(v[1]);
        f2 = v[0].cross(v[2]);
        Vector3f p(x,y,1.);
        if((p.dot(f0)*f0.dot(v[2])>0) && (p.dot(f1)*f1.dot(v[0])>0) && (p.dot(f2)*f2.dot(v[1])>0))
            return true;
        return false;
    }

    static std::tuple<float, float, float> computeBarycentric2D(float x, float y, const Vector3f *v)
    {
        float c1 = (x * (v[1].y() - v[2].y()) + (v[2].x() - v[1].x()) * y + v[1].x() * v[2].y() - v[2].x() * v[1].y()) / (v[0].x() * (v[1].y() - v[2].y()) + (v[2].x() - v[1].x()) * v[0].y() + v[1].x() * v[2].y() - v[2].x() * v[1].y());
        float c2 = (x * (v[2].y() - v[0].y()) + (v[0].x() - v[2].x()) * y + v[2].x() * v[0].y() - v[0].x() * v[2].y()) / (v[1].x() * (v[2].y() - v[0].y()) + (v[0].x() - v[2].x()) * v[1].y() + v[2].x() * v[0].y() - v[0].x() * v[2].y());
        float c3 = (x * (v[0].y() - v[1].y()) + (v[1].x() - v[0].x()) * y + v[0].x() * v[1].y() - v[1].x() * v[0].y()) / (v[2].x() * (v[0].y() - v[1].y()) + (v[1].x() - v[0].x()) * v[2].y() + v[0].x() * v[1].y() - v[1].x() * v[0].y());
        return {c1, c2, c3};
    }

    void rst::rasterizer::draw(pos_buf_id pos_buffer, ind_buf_id ind_buffer, col_buf_id col_buffer, Primitive type)
    {
        auto &buf = pos_buf[pos_buffer.pos_id];
        auto &ind = ind_buf[ind_buffer.ind_id];
        auto &col = col_buf[col_buffer.col_id];

        float f1 = (50 - 0.1) / 2.0;
        float f2 = (50 + 0.1) / 2.0;

        Eigen::Matrix4f mvp = projection * view * model;
        for (auto &i : ind)
        {
            Triangle t;
            Eigen::Vector4f v[] = {
                mvp * to_vec4(buf[i[0]], 1.0f),
                mvp * to_vec4(buf[i[1]], 1.0f),
                mvp * to_vec4(buf[i[2]], 1.0f)};
            // Homogeneous division
            for (auto &vec : v)
            {
                vec /= vec.w();
            }
            // Viewport transformation
            for (auto &vert : v)
            {
                vert.x() = 0.5 * width * (vert.x() + 1.0);
                vert.y() = 0.5 * height * (vert.y() + 1.0);
                vert.z() = vert.z() * f1 + f2;
            }

            for (int i = 0; i < 3; ++i)
            {
                t.setVertex(i, v[i].head<3>());
                t.setVertex(i, v[i].head<3>());
                t.setVertex(i, v[i].head<3>());
            }

            auto col_x = col[i[0]];
            auto col_y = col[i[1]];
            auto col_z = col[i[2]];

            t.setColor(0, col_x[0], col_x[1], col_x[2]);
            t.setColor(1, col_y[0], col_y[1], col_y[2]);
            t.setColor(2, col_z[0], col_z[1], col_z[2]);

            rasterize_triangle(t);
        }
    }

    // Screen space rasterization
    void rst::rasterizer::rasterize_triangle(const Triangle &t)
    {
        auto v = t.toVector4();

        // TODO : Find out the bounding box of current triangle.
        float minX = v[0].x(), minY = v[0].y();
        float maxX = v[0].x(), maxY = v[0].y();
        for (const auto &vec : v)
        {
            minX = std::min(minX, vec.x());
            minY = std::min(minY, vec.y());
            maxX = std::max(maxX, vec.x());
            maxY = std::max(maxY, vec.y());
        }
        // iterate through the pixel and find if the current pixel is inside the triangle

        for (int x = int(std::floor(minX)); x <= int(std::ceil(maxX)); ++x)
        {
            for (int y = int(std::floor(minY)); y <= int(std::ceil(maxY)); ++y)
            {
                if (super_sample > 0)
                {
                    Vector3f color(0,0,0);
                    // super-sampling
                    int sample_x = super_sample;
                    int sample_y = super_sample;
                    float cx = 1.0f / sample_x * 0.5f, cy = 1.0f / sample_y * 0.5f;
                    for (int i = 0; i < sample_x; ++i)
                    {
                        float dx = 1.0f / sample_x * i;
                        for (int j = 0; j < sample_y; ++j)
                        {
                            float dy = 1.0f / sample_y * j;
                            float xx = x + dx, yy = y + dy;
                            int id = get_index(int(std::round(xx * sample_x)), int(std::round(yy * sample_y)));
                            if (!insideTriangle(xx + cx, yy + cy, t.v))
                            {
                                color += color_buf[id];
                                continue;
                            }
                            auto [alpha, beta, gamma] = computeBarycentric2D(xx, yy, t.v);
                            float w_reciprocal = 1.0 / (alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
                            float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
                            z_interpolated *= w_reciprocal;
                            if (z_interpolated < this->depth_buf[id])
                            {
                                this->depth_buf[id] = z_interpolated;
                                color_buf[id] = t.getColor();
                            }
                            color += color_buf[id];
                        }
                    }
                    color = color / (1.0f * super_sample * super_sample);
                    this->set_pixel(Eigen::Vector3f(x, y, 1), color);
                }
                else
                {
                    if (!insideTriangle(x + 0.5f, y + 0.5f, t.v))
                        continue;
                    // If so, use the following code to get the interpolated z value.
                    auto [alpha, beta, gamma] = computeBarycentric2D(x, y, t.v);
                    float w_reciprocal = 1.0 / (alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
                    float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
                    z_interpolated *= w_reciprocal;
                    // TODO : set the current pixel (use the set_pixel function) to the color of the triangle (use getColor function) if it should be painted.
                    int id = get_index(x, y);
                    if (z_interpolated < this->depth_buf[id])
                    {
                        this->set_pixel(Eigen::Vector3f(x, y, 1), t.getColor());
                        this->depth_buf[id] = z_interpolated;
                    }
                }
            }
        }
    }

    void rst::rasterizer::set_model(const Eigen::Matrix4f &m)
    {
        model = m;
    }

    void rst::rasterizer::set_view(const Eigen::Matrix4f &v)
    {
        view = v;
    }

    void rst::rasterizer::set_projection(const Eigen::Matrix4f &p)
    {
        projection = p;
    }

    void rst::rasterizer::clear(rst::Buffers buff)
    {
        if ((buff & rst::Buffers::Color) == rst::Buffers::Color)
        {
            std::fill(frame_buf.begin(), frame_buf.end(), Eigen::Vector3f{0, 0, 0});
        }
        if ((buff & rst::Buffers::Depth) == rst::Buffers::Depth)
        {
            std::fill(depth_buf.begin(), depth_buf.end(), std::numeric_limits<float>::infinity());
        }
    }

    rst::rasterizer::rasterizer(int w, int h, int ss) : width(w), height(h), super_sample(ss)
    {
        if (super_sample == 0)
        {
            frame_buf.resize(w * h);
            depth_buf.resize(w * h, INFINITY);
        }
        else
        {
            frame_buf.resize(w * h);
            depth_buf.resize(w * h * super_sample * super_sample, INFINITY);
            color_buf.resize(w * h * super_sample * super_sample, Vector3f(0, 0, 0));
        }
    }

    int rst::rasterizer::get_index(int x, int y)
    {
        if (super_sample > 0)
        {
            return (height * super_sample - 1 - y) * width * super_sample + x;
        }
        return (height - 1 - y) * width + x;
    }

    void rst::rasterizer::set_pixel(const Eigen::Vector3f &point, const Eigen::Vector3f &color)
    {
        // old index: auto ind = point.y() + point.x() * width;
        auto ind = (height - 1 - point.y()) * width + point.x();
        frame_buf[ind] = color;
    }

    // clang-format on
}
