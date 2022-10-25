#pragma once

#include <vector>
#include "Vector.hpp"
#include "Object.hpp"
#include "Light.hpp"
#include "AreaLight.hpp"
#include "BVH.hpp"
#include "Ray.hpp"

class Scene
{
public:
    // setting up options
    int width = 1280;
    int height = 960;
    double fov = 40;
    Vector3f backgroundColor = Vector3f(0.235294, 0.67451, 0.843137);
    int maxDepth = 1;
    float RussianRoulette = 0.8;

    Scene(int w, int h) : width(w), height(h) {}

    void Add(Object *object) { objects.push_back(object); }
    void Add(std::unique_ptr<Light> light) { lights.push_back(std::move(light)); }

    const std::vector<Object *> &get_objects() const { return objects; }
    const std::vector<std::unique_ptr<Light>> &get_lights() const { return lights; }
    Intersection intersect(const Ray &ray) const;
    BVHAccel *bvh;
    void buildBVH();
    Vector3f castRay(const Ray &ray, int depth) const;
    void sampleLight(Intersection &pos, float &pdf) const;
    bool trace(const Ray &ray, const std::vector<Object *> &objects, float &tNear, uint32_t &index, Object **hitObject);
    std::tuple<Vector3f, Vector3f> HandleAreaLight(const AreaLight &light, const Vector3f &hitPoint, const Vector3f &N,
                                                   const Vector3f &shadowPointOrig,
                                                   const std::vector<Object *> &objects, uint32_t &index,
                                                   const Vector3f &dir, float specularExponent);

    // creating the scene (adding objects and lights)
    std::vector<Object *> objects;
    std::vector<std::unique_ptr<Light>> lights;

    // Compute reflection direction
    Vector3f reflect(const Vector3f &I, const Vector3f &N) const
    {
        return I - 2 * dotProduct(I, N) * N;
    }

    // Compute refraction direction using Snell's law
    //
    // We need to handle with care the two possible situations:
    //
    //    - When the ray is inside the object
    //
    //    - When the ray is outside.
    //
    // If the ray is outside, you need to make cosi positive cosi = -N.I
    //
    // If the ray is inside, you need to invert the refractive indices and negate the normal N
    Vector3f refract(const Vector3f &I, const Vector3f &N, const float &ior) const
    {
        float cosi = clamp(-1, 1, dotProduct(I, N));
        float etai = 1, etat = ior;
        Vector3f n = N;
        if (cosi < 0)
        {
            cosi = -cosi;
        }
        else
        {
            std::swap(etai, etat);
            n = -N;
        }
        float eta = etai / etat;
        float k = 1 - eta * eta * (1 - cosi * cosi);
        return k < 0 ? 0 : eta * I + (eta * cosi - sqrtf(k)) * n;
    }

    // Compute Fresnel equation
    //
    // \param I is the incident view direction
    //
    // \param N is the normal at the intersection point
    //
    // \param ior is the material refractive index
    //
    // \param[out] kr is the amount of light reflected
    void fresnel(const Vector3f &I, const Vector3f &N, const float &ior, float &kr) const
    {
        float cosi = clamp(-1, 1, dotProduct(I, N));
        float etai = 1, etat = ior;
        if (cosi > 0)
        {
            std::swap(etai, etat);
        }
        // Compute sini using Snell's law
        float sint = etai / etat * sqrtf(std::max(0.f, 1 - cosi * cosi));
        // Total internal reflection
        if (sint >= 1)
        {
            kr = 1;
        }
        else
        {
            float cost = sqrtf(std::max(0.f, 1 - sint * sint));
            cosi = fabsf(cosi);
            float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
            float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
            kr = (Rs * Rs + Rp * Rp) / 2;
        }
        // As a consequence of the conservation of energy, transmittance is given by:
        // kt = 1 - kr;
    }
};

void Scene::buildBVH()
{
    printf(" - Generating BVH...\n\n");
    this->bvh = new BVHAccel(objects, 1, BVHAccel::SplitMethod::NAIVE);
}

Intersection Scene::intersect(const Ray &ray) const
{
    return this->bvh->Intersect(ray);
}

void Scene::sampleLight(Intersection &pos, float &pdf) const
{
    float emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k)
    {
        if (objects[k]->hasEmit())
        {
            emit_area_sum += objects[k]->getArea();
        }
    }
    float p = get_random_float() * emit_area_sum;
    emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k)
    {
        if (objects[k]->hasEmit())
        {
            emit_area_sum += objects[k]->getArea();
            if (p <= emit_area_sum)
            {
                objects[k]->Sample(pos, pdf);
                break;
            }
        }
    }
}

bool Scene::trace(
    const Ray &ray,
    const std::vector<Object *> &objects,
    float &tNear, uint32_t &index, Object **hitObject)
{
    *hitObject = nullptr;
    for (uint32_t k = 0; k < objects.size(); ++k)
    {
        float tNearK = kInfinity;
        uint32_t indexK;
        Vector2f uvK;
        if (objects[k]->intersect(ray, tNearK, indexK) && tNearK < tNear)
        {
            *hitObject = objects[k];
            tNear = tNearK;
            index = indexK;
        }
    }

    return (*hitObject != nullptr);
}

// Implementation of Path Tracing
Vector3f Scene::castRay(const Ray &ray, int depth) const
{
    // TO DO Implement Path Tracing Algorithm here
    Intersection inter_obj = this->intersect(ray);

    if (!inter_obj.happened) // 若光线与场景没有交点，返回0
        return Vector3f();
    
    if (inter_obj.m->hasEmission()) // 若光线打到光源，则返回emission
        return inter_obj.m->getEmission();
    
    Vector3f L_dir, L_indir;

    switch (inter_obj.m->getType())
    {
    case DIFFUSE: //漫反射
    {
        // 采样光源
        Intersection inter_light;
        float pdf_light;
        this->sampleLight(inter_light, pdf_light);

        Vector3f obj2light = inter_light.coords - inter_obj.coords;
        Vector3f obj2light_dir = obj2light.normalized();
        if (this->intersect(Ray(inter_obj.coords, obj2light_dir)).distance - obj2light.norm() > -EPSILON)
        {
            L_dir = inter_light.emit * 
            inter_obj.m->eval(ray.direction, obj2light_dir, inter_obj.normal) * 
            dotProduct(obj2light_dir, inter_obj.normal) *
            dotProduct(-obj2light_dir, inter_light.normal) /
            dotProduct(obj2light, obj2light) / 
            pdf_light;
        }
    }
    case GLOSSY:
    {
        // 俄罗斯轮盘赌
        if (get_random_float() <= RussianRoulette)
        {
            Vector3f obj2nobj_dir = inter_obj.m->sample(ray.direction, inter_obj.normal).normalized();
            Ray nray(inter_obj.coords, obj2nobj_dir);
            Intersection nextObjInter = this->intersect(nray);
            // 若光线命中非光源的物体
            if (nextObjInter.happened && !nextObjInter.m->hasEmission()) 
            {
                // 计算概率密度函数值PDF
                float pdf = inter_obj.m->pdf(ray.direction, obj2nobj_dir, inter_obj.normal);
                if (pdf > EPSILON)
                {
                    L_indir = castRay(nray, depth + 1) * 
                    inter_obj.m->eval(ray.direction, obj2nobj_dir, inter_obj.normal) * 
                    dotProduct(obj2nobj_dir, inter_obj.normal) / 
                    pdf / 
                    RussianRoulette;
                }
            }
        }
        break;
    }
    default:
        break;
    }

    return L_dir + L_indir;
}