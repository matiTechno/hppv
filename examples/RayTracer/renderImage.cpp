#include <limits>

#include <glm/trigonometric.hpp>
#include <glm/geometric.hpp>

#include "renderImage.hpp"

// note: we are using doubles for calculations

struct Camera
{
    glm::dvec3 eye = {0.0, 0.0, 0.0};
    glm::dvec3 center = {0.0, 0.0, -1.0};
    glm::dvec3 up = {0.0, 1.0, 0.0};
    double fov = 90.0; // in degrees
};

struct Ray
{
    glm::dvec3 origin;
    glm::dvec3 dir;
};

struct Sphere
{
    glm::dvec3 center;
    double radius;
    glm::dvec3 color;
};

Pixel toPixel(const glm::dvec3 v)
{
    Pixel p;
    p.r = static_cast<unsigned char>(v.r * 255.0);
    p.g = static_cast<unsigned char>(v.g * 255.0);
    p.b = static_cast<unsigned char>(v.b * 255.0);
    return p;
}

bool intersect(Ray ray, const Sphere& sphere, double* const distance)
{
    // todo: vector naming

    const auto L = sphere.center - ray.origin;
    const auto tca = glm::dot(ray.dir, L);
    const auto d2 = glm::dot(L, L)  - tca * tca;
    const auto radius2 = sphere.radius * sphere.radius;

    if(d2 > radius2)
        return false;

    const auto thc = glm::sqrt(radius2 - d2);
    const auto t0 = tca - thc;

    if(t0 >= 0)
    {
        *distance = t0;
        return true;
    }
    else
        return false;
}

void renderImage(Pixel* buffer, const glm::ivec2 size, std::atomic_int& progress)
{
    Camera camera;
    camera.eye = {-1.0, 3.0, 1.0};

    // camera coordinate system
    const auto z = glm::normalize(camera.eye - camera.center);
    const auto x = glm::normalize(glm::cross(camera.up, z));
    const auto y = glm::cross(z, x);

    const Sphere spheres[] =
    {
        {{0.0, 0.0, -2.0}, 1.0, {1.0, 0.5, 0.0}},
        {{-0.8, 0.0, -2.0}, 1.0, {0.0, 0.0, 1.0}},
        {{0.8, 0.0, -2.0}, 1.0, {0.0, 1.0, 0.0}},
        {{-1.0, 1.5, -1.5}, 0.5, {1.f, 1.f, 1.f}}
    };

    const auto aspectRatio = static_cast<double>(size.x) / size.y;
    const auto scale = glm::tan(glm::radians(camera.fov / 2.0));

    Ray ray;
    ray.origin = camera.eye;

    for(auto j = 0; j < size.y; ++j)
    {
        for(auto i = 0; i < size.x; ++i)
        {
            auto distance = std::numeric_limits<double>::infinity();
            const Sphere* hitSphere = nullptr;

            glm::dvec3 rayScreenPos;
            rayScreenPos.x = (2.0 * (i + 0.5) / size.x - 1.0) * scale * aspectRatio;
            rayScreenPos.y = (1.0 - 2.0 * (j + 0.5) / size.y) * scale;
            rayScreenPos.z = -1.0;

            ray.dir = glm::normalize(x * rayScreenPos.x + y * rayScreenPos.y + z * rayScreenPos.z);

            for(const auto& sphere: spheres)
            {
                double t;

                if(intersect(ray, sphere, &t) && (t < distance))
                {
                    distance = t;
                    hitSphere = &sphere;
                }
            }

            if(hitSphere)
            {
                const auto hitPoint = ray.origin + ray.dir * distance;
                const auto normal = glm::normalize(hitPoint - hitSphere->center);
                const auto dot = glm::dot(-ray.dir, normal);
                *buffer = toPixel(hitSphere->color * dot);
            }

            ++buffer;
            ++progress;
        }
    }
}
