#pragma once

#include "Scene.hpp"
#include "Renderer.hpp"

#include <fstream>
#include <omp.h>

struct hit_payload
{
    float tNear;
    uint32_t index;
    Vector2f uv;
    Object *hit_obj;
};

class Renderer
{
public:
    void Render(const Scene &scene, int spp);
    void Render(const Scene &scene, int spp, const int num_workers);

private:
};

inline float deg2rad(const float &deg) { return deg * M_PI / 180.0; }

/**
 * \brief 修改了EPSILON的值，0.00016渲染效果更加美观
 */
const float EPSILON = 0.00016;

// The main render function. This where we iterate over all pixels in the image,
// generate primary rays and cast these rays into the scene. The content of the
// framebuffer is saved to a file.
void Renderer::Render(const Scene &scene, int spp)
{
    std::vector<Vector3f> framebuffer(scene.width * scene.height);

    float scale = tan(deg2rad(scene.fov * 0.5));
    float imageAspectRatio = scene.width / (float)scene.height;
    Vector3f eye_pos(278, 273, -800);
    int m = 0;

    // change the spp value to change sample ammount
    std::cout << "SPP: " << spp << "\n";

    int width = std::sqrt(1.0 * spp * scene.width / scene.height);
    int height = std::sqrt(1.0 * spp * scene.height / scene.width);

    float wstep = 1.0f / width;
    float hstep = 1.0f / height;
    
    for (uint32_t j = 0; j < scene.height; ++j)
    {
        for (uint32_t i = 0; i < scene.width; ++i)
        {
            // generate primary ray direction
            for (int k = 0; k < spp; k++)
            {
                // 使用MSAA反走样
                float x = (2 * (i + wstep / 2 + wstep * (k % width)) / (float)scene.width - 1) *
                        imageAspectRatio * scale;
                float y = (1 - 2 * (j + hstep / 2 + hstep * (k / height)) / (float)scene.height) * scale;

                Vector3f dir = normalize(Vector3f(-x, y, 1));
                framebuffer[m] += scene.castRay(Ray(eye_pos, dir), 0) / spp;
            }
            m++;
        }
        UpdateProgress(j / (float)scene.height);
    }
    UpdateProgress(1.f);

    // save framebuffer to file
    FILE *fp = fopen("binary.ppm", "wb");
    (void)fprintf(fp, "P6\n%d %d\n255\n", scene.width, scene.height);
    for (auto i = 0; i < scene.height * scene.width; ++i)
    {
        static unsigned char color[3];
        color[0] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].x), 0.6f));
        color[1] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].y), 0.6f));
        color[2] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].z), 0.6f));
        fwrite(color, 1, 3, fp);
    }
    fclose(fp);
}

/**
 * @brief 路径追踪 cpu多线程并行 渲染（通过MSAA抗锯齿）
 * @param scene         待渲染的场景
 * @param spp           每个像素采样数目
 * @param num_workers   并行线程数目
 */
void Renderer::Render(const Scene &scene, int spp, const int num_workers)
{
    std::vector<Vector3f> framebuffer(scene.width * scene.height);

    float scale = tan(deg2rad(scene.fov * 0.5));
    float imageAspectRatio = scene.width / (float)scene.height;
    Vector3f eye_pos(278, 273, -800);

    // change the spp value to change sample ammount
    std::cout << "SPP: " << spp << " num_workers: " << num_workers << "\n";
    int prog = 0;

    int width = std::sqrt(1.0 * spp * scene.width / scene.height);
    int height = std::sqrt(1.0 * spp * scene.height / scene.width);

    float wstep = 1.0f / width;
    float hstep = 1.0f / height;

    omp_set_num_threads(num_workers);
    
#pragma omp parallel for
    for (uint32_t j = 0; j < scene.height; ++j)
    {
        for (uint32_t i = 0; i < scene.width; ++i)
        {
            // generate primary ray direction
            int m = j * scene.width + i;

            for (int k = 0; k < spp; k++)
            {
                // 使用MSAA反走样
                float x = (2 * (i + wstep / 2 + wstep * (k % width)) / (float)scene.width - 1) *
                        imageAspectRatio * scale;
                float y = (1 - 2 * (j + hstep / 2 + hstep * (k / height)) / (float)scene.height) * scale;

                Vector3f dir = normalize(Vector3f(-x, y, 1));
                framebuffer[m] += scene.castRay(Ray(eye_pos, dir), 0) / spp;
            }

        }
#pragma omp critical
        {
            UpdateProgress(prog / (float)scene.height);
            prog++;
        }
    }
    UpdateProgress(1.f);

    // save framebuffer to file
    FILE *fp = fopen("binary.ppm", "wb");
    (void)fprintf(fp, "P6\n%d %d\n255\n", scene.width, scene.height);
    for (auto i = 0; i < scene.height * scene.width; ++i)
    {
        static unsigned char color[3];
        color[0] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].x), 0.6f));
        color[1] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].y), 0.6f));
        color[2] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].z), 0.6f));
        fwrite(color, 1, 3, fp);
    }
    fclose(fp);
}