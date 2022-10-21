#pragma once

#include "Vector.hpp"
#include "Object.hpp"
#include "Light.hpp"
#include "AreaLight.hpp"
#include "BVH.hpp"
#include "Ray.hpp"

#include <vector>

class Scene
{
public:
    // setting up options
    int width = 1280;
    int height = 960;
    double fov = 90;
    Vector3f backgroundColor = Vector3f(0.235294, 0.67451, 0.843137);
    int maxDepth = 5;

    Scene(int w, int h) : width(w), height(h) {}

    void Add(Object *object) { objects.push_back(object); }
    void Add(std::unique_ptr<Light> light) { lights.push_back(std::move(light)); }

    const std::vector<Object *> &get_objects() const { return objects; }
    const std::vector<std::unique_ptr<Light>> &get_lights() const { return lights; }
    Intersection intersect(const Ray &ray) const;
    BVHAccel *bvh;
    void buildBVH();
    Vector3f castRay(const Ray &ray, int depth) const;
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

// Implementation of the Whitted-syle light transport algorithm (E [S*] (D|G) L)
//
// This function is the function that compute the color at the intersection point
// of a ray defined by a position and a direction. Note that thus function is recursive (it calls itself).
//
// If the material of the intersected object is either reflective or reflective and refractive,
// then we compute the reflection/refracton direction and cast two new rays into the scene
// by calling the castRay() function recursively. When the surface is transparent, we mix
// the reflection and refraction color using the result of the fresnel equations (it computes
// the amount of reflection and refractin depending on the surface normal, incident view direction
// and surface refractive index).
//
// If the surface is duffuse/glossy we use the Phong illumation model to compute the color
// at the intersection point.
Vector3f Scene::castRay(const Ray &ray, int depth) const
{
    if (depth > this->maxDepth)
    {
        return Vector3f(0.0, 0.0, 0.0);
    }
    Intersection intersection = Scene::intersect(ray);
    Material *m = intersection.m;
    Object *hitObject = intersection.obj;
    Vector3f hitColor = this->backgroundColor;
    //    float tnear = kInfinity;
    Vector2f uv;
    uint32_t index = 0;
    if (intersection.happened)
    {

        Vector3f hitPoint = intersection.coords;
        Vector3f N = intersection.normal; // normal
        Vector2f st;                      // st coordinates
        hitObject->getSurfaceProperties(hitPoint, ray.direction, index, uv, N, st);
        //        Vector3f tmp = hitPoint;
        switch (m->getType())
        {
        case REFLECTION_AND_REFRACTION:
        {
            Vector3f reflectionDirection = normalize(reflect(ray.direction, N));
            Vector3f refractionDirection = normalize(refract(ray.direction, N, m->ior));
            Vector3f reflectionRayOrig = (dotProduct(reflectionDirection, N) < 0) ? hitPoint - N * EPSILON : hitPoint + N * EPSILON;
            Vector3f refractionRayOrig = (dotProduct(refractionDirection, N) < 0) ? hitPoint - N * EPSILON : hitPoint + N * EPSILON;
            Vector3f reflectionColor = castRay(Ray(reflectionRayOrig, reflectionDirection), depth + 1);
            Vector3f refractionColor = castRay(Ray(refractionRayOrig, refractionDirection), depth + 1);
            float kr;
            fresnel(ray.direction, N, m->ior, kr);
            hitColor = reflectionColor * kr + refractionColor * (1 - kr);
            break;
        }
        case REFLECTION:
        {
            float kr;
            fresnel(ray.direction, N, m->ior, kr);
            Vector3f reflectionDirection = reflect(ray.direction, N);
            Vector3f reflectionRayOrig = (dotProduct(reflectionDirection, N) < 0) ? hitPoint + N * EPSILON : hitPoint - N * EPSILON;
            hitColor = castRay(Ray(reflectionRayOrig, reflectionDirection), depth + 1) * kr;
            break;
        }
        default:
        {
            // [comment]
            // We use the Phong illumation model int the default case. The phong model
            // is composed of a diffuse and a specular reflection component.
            // [/comment]
            Vector3f lightAmt = 0, specularColor = 0;
            Vector3f shadowPointOrig = (dotProduct(ray.direction, N) < 0) ? hitPoint + N * EPSILON : hitPoint - N * EPSILON;
            // [comment]
            // Loop over all lights in the scene and sum their contribution up
            // We also apply the lambert cosine law
            // [/comment]
            for (uint32_t i = 0; i < get_lights().size(); ++i)
            {
                auto area_ptr = dynamic_cast<AreaLight *>(this->get_lights()[i].get());
                if (area_ptr)
                {
                    // Do nothing for this assignment
                }
                else
                {
                    Vector3f lightDir = get_lights()[i]->position - hitPoint;
                    // square of the distance between hitPoint and the light
                    float lightDistance2 = dotProduct(lightDir, lightDir);
                    lightDir = normalize(lightDir);
                    float LdotN = std::max(0.f, dotProduct(lightDir, N));
                    Object *shadowHitObject = nullptr;
                    float tNearShadow = kInfinity;
                    // is the point in shadow, and is the nearest occluding object closer to the object than the light itself?
                    bool inShadow = bvh->Intersect(Ray(shadowPointOrig, lightDir)).happened;
                    lightAmt += (1 - inShadow) * get_lights()[i]->intensity * LdotN;
                    Vector3f reflectionDirection = reflect(-lightDir, N);
                    specularColor += powf(std::max(0.f, -dotProduct(reflectionDirection, ray.direction)),
                                          m->specularExponent) *
                                     get_lights()[i]->intensity;
                }
            }
            hitColor = lightAmt * (hitObject->evalDiffuseColor(st) * m->Kd + specularColor * m->Ks);
            break;
        }
        }
    }

    return hitColor;
}