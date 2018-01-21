#include <limits>

#include <glm/trigonometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "renderImage.hpp"

struct Camera
{
    glm::vec3 eye = {0.f, 0.f, 0.f};
    glm::vec3 at = {0.f, 0.f, -1.f};
    glm::vec3 up = {0.f, 1.f, 0.f};
    float fov = 45.f; // in degrees
};

struct Ray
{
    glm::vec3 origin;
    glm::vec3 dir;
};

struct Sphere
{
    glm::vec3 center;
    float radius;
    glm::vec3 color;
};

Pixel toPixel(const glm::vec3 v)
{
    Pixel p;
    p.r = v.r * 255.f;
    p.g = v.g * 255.f;
    p.b = v.b * 255.f;
    return p;
}

// something is wrong with this

Ray getRayInWorld(const glm::ivec2 pixelPos, const glm::ivec2 imageSize, const Camera& camera)
{
    const auto ndcPos = (glm::vec2(pixelPos) + 0.5f) / glm::vec2(imageSize);
    glm::vec3 screenPos(ndcPos * 2.f - 1.f, -1.f);

    // invert y
    screenPos.y = 1 - 2 * ndcPos.y;

    // aspect ratio correction
    screenPos.x *= static_cast<float>(imageSize.x) / imageSize.y;

    screenPos *= glm::tan(glm::radians(camera.fov) / 2.f);

    const glm::vec3 origin(0.f);
    const auto dir = glm::normalize(screenPos - origin);

    const auto worldFromView = glm::inverse(glm::lookAt(camera.eye, camera.at - camera.eye, camera.up));

    return {worldFromView * glm::vec4(origin, 1.f), glm::normalize(worldFromView * glm::vec4(dir, 1.f))};
}

bool intersect(Ray ray, const Sphere sphere, float* const distance)
{
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
    {
        return false;
    }
}

void renderImage(Pixel* buffer, const glm::ivec2 size, std::atomic_int& progress)
{
    const Camera camera;

    const Sphere spheres[] =
    {
        {{0.f, 0.f, -1.24f}, 1.f, {1.f, 0.5f, 0.f}},
        {{0.5f, 0.f, -1.2f}, 1.f, {0.f, 1.f, 0.f}},
        {{-0.5f, 0.f, -1.2f}, 1.f, {0.f, 0.f, 1.f}}
    };

    for(auto j = 0; j < size.y; ++j)
    {
        for(auto i = 0; i < size.x; ++i)
        {
            float distance = std::numeric_limits<float>::infinity();
            const Sphere* hitSphere = nullptr;
            const auto ray = getRayInWorld({i, j}, size, camera);

            for(const auto& sphere: spheres)
            {
                float t;

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
                const auto a = glm::dot(-ray.dir, normal);
                *buffer = toPixel(hitSphere->color * a);
            }

            ++buffer;
            ++progress;
        }
    }
}
