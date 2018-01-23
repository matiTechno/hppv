#include <limits>

#include <glm/trigonometric.hpp>
#include <glm/geometric.hpp>

#include "renderImage.hpp"

// note: we are using doubles for calculations
// todo: better vector naming

struct Camera
{
    glm::dvec3 eye = {0.0, 0.0, 0.0};
    glm::dvec3 center = {0.0, 0.0, -1.0};
    glm::dvec3 up = {0.0, 1.0, 0.0};
    double fovy = 90.0; // in degrees
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

struct Plane
{
    glm::dvec3 center;
    glm::dvec3 normal;
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

bool intersect(const Ray ray, const Sphere& sphere, double* const distance)
{
    const auto L = sphere.center - ray.origin;
    const auto tca = glm::dot(ray.dir, L);
    const auto d2 = glm::dot(L, L)  - tca * tca;
    const auto radius2 = sphere.radius * sphere.radius;

    if(d2 > radius2)
        return false;

    const auto thc = glm::sqrt(radius2 - d2);
    const auto t0 = tca - thc;

    if(t0 < 0.0)
        return false;

    *distance = t0;
    return true;
}

bool intersect(const Ray ray, const Plane& plane, double* const distance)
{
    const auto denominator = glm::dot(ray.dir, plane.normal);

    if(glm::abs(denominator) < 0.001)
        return false;

    const auto t = glm::dot(plane.center - ray.origin, plane.normal) / denominator;

    if(t < 0.0)
        return false;

    *distance = t;
    return true;
}

void renderImage1(Pixel* buffer, const glm::ivec2 size, std::atomic_int& progress)
{
    Camera camera;
    camera.eye = {-1.0, 1.5, 2.0};

    // camera coordinate system
    const auto z = glm::normalize(camera.eye - camera.center);
    const auto x = glm::normalize(glm::cross(camera.up, z));
    const auto y = glm::cross(z, x);

    const Sphere spheres[] =
    {
        {{0.0, 0.0, -2.0}, 1.0, {1.0, 0.5, 0.0}},
        {{-0.8, 0.0, -2.0}, 1.0, {0.0, 0.0, 1.0}},
        {{0.8, 0.0, -2.0}, 1.0, {0.0, 1.0, 0.0}},
        {{-1.0, 1.5, -1.5}, 0.5, {1.0, 1.0, 1.0}}
    };

    Plane planes [] =
    {
        {{0.0, -10.0, 0.0}, {0.0, 1.0, 0.0}, {1.0, 0.0, 0.0}},
        {{-50.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}}
    };

    for(auto& plane: planes)
    {
        plane.normal = glm::normalize(plane.normal);
    }

    const auto aspectRatio = static_cast<double>(size.x) / size.y;
    const auto scale = glm::tan(glm::radians(camera.fovy / 2.0));

    Ray ray;
    ray.origin = camera.eye;

    for(auto j = 0; j < size.y; ++j)
    {
        for(auto i = 0; i < size.x; ++i)
        {
            glm::dvec3 rayScreenPos;
            rayScreenPos.x = (2.0 * (i + 0.5) / size.x - 1.0) * scale * aspectRatio;
            rayScreenPos.y = (1.0 - 2.0 * (j + 0.5) / size.y) * scale;
            rayScreenPos.z = -1.0;

            ray.dir = glm::normalize(x * rayScreenPos.x + y * rayScreenPos.y + z * rayScreenPos.z);

            auto distance = std::numeric_limits<double>::infinity();

            // it doesn't scale very well, I know
            // at the end we will triangulate everything

            enum class Object
            {
                Sphere,
                Plane
            };

            struct
            {
                const void* object = nullptr;
                Object type;
            }
            hit;

            for(const auto& sphere: spheres)
            {
                double t;

                if(intersect(ray, sphere, &t) && (t < distance))
                {
                    distance = t;
                    hit.object = &sphere;
                    hit.type = Object::Sphere;
                }
            }

            for(const auto& plane: planes)
            {
                double t;

                if(intersect(ray, plane, &t) && (t < distance))
                {
                    distance = t;
                    hit.object = &plane;
                    hit.type = Object::Plane;
                }
            }

            if(hit.object)
            {
                switch(hit.type)
                {
                case Object::Sphere:
                {
                    const auto hitPoint = ray.origin + ray.dir * distance;
                    const auto& sphere = *reinterpret_cast<const Sphere*>(hit.object);
                    const auto normal = glm::normalize(hitPoint - sphere.center);
                    const auto dot = glm::dot(-ray.dir, normal);
                    *buffer = toPixel(sphere.color * dot);
                    break;
                }
                case Object::Plane:
                {
                    const auto& plane = *reinterpret_cast<const Plane*>(hit.object);
                    const auto dot = glm::dot(-ray.dir, plane.normal);
                    *buffer = toPixel(plane.color * glm::abs(dot));
                }
                }
            }

            ++buffer;
            ++progress;
        }
    }
}

// ###
// github.com/ssloy/tinyrenderer/wiki

#include <cassert>
#include <utility> // std::swap
#include <vector>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

void line(glm::ivec2 start, glm::ivec2 end, const glm::dvec3 color,
          Pixel* const buffer, const glm::ivec2 imageSize)
{
    assert(start.x < imageSize.x && start.x >= 0);
    assert(start.y < imageSize.y && start.y >= 0);
    assert(end.x < imageSize.x && end.x >= 0);
    assert(end.y < imageSize.y && end.y >= 0);

    // tinyrenderer doesn't have this check, strange
    if(start == end)
        return;

    auto steep = false;

    if(glm::abs(end.x - start.x) < glm::abs(end.y - start.y))
    {
        std::swap(start.x, start.y);
        std::swap(end.x, end.y);
        steep = true;
    }

    if(start.x > end.x)
    {
        std::swap(start, end);
    }


    for(auto x = start.x; x <= end.x; ++x)
    {
        const auto t = static_cast<double>(x - start.x) / (end.x - start.x);
        const auto y = start.y * (1.0 - t) + end.y * t;
        glm::ivec2 pixelPos(x, y);

        if(steep)
        {
            std::swap(pixelPos.x, pixelPos.y);
        }

        *(buffer + pixelPos.y * imageSize.x + pixelPos.x) = toPixel(color);
    }
}

struct Model
{
    Model(const char* const filename)
    {
        std::fstream file(filename);

        if(!file.is_open())
        {
            std::cout << "Model: could not open file " << filename << std::endl;
            return;
        }

        std::string line;

        while(std::getline(file, line))
        {
            std::stringstream s(line);
            std::string t;
            s >> t;

            if(t == "v")
            {
                glm::dvec3 vertex;

                for(auto i = 0; i < 3; ++i)
                {
                    s >> vertex[i];
                }

                vertices.push_back(vertex);
            }
            else if(t == "f")
            {
                glm::ivec3 face;
                int idx; // we need it because glm assertion fails on face[3] (while loop)
                int i = 0;
                int dummyInt;
                char dummyChar;

                while(s >> idx >> dummyChar >> dummyInt >> dummyChar >> dummyInt)
                {
                    assert(i < 3);
                    face[i] = --idx; // in wavefront obj all indices start at 1, not 0
                    ++i;
                }

                assert(i == 3);
                faces.push_back(face);
            }
        }
    }

    std::vector<glm::dvec3> vertices;
    std::vector<glm::ivec3> faces;
};

void renderImage2(Pixel* const buffer, const glm::ivec2 size, std::atomic_int& progress)
{
    const auto bufferSize = size.x * size.y;

    for(auto i = 0; i < bufferSize; ++i)
    {
        *(buffer + i) = {40, 0, 0};
    }

    Model model("res/african_head.obj");

    for(const auto& face: model.faces)
    {
        for(auto i = 0; i < 3; ++i)
        {
            const auto v0 = model.vertices[face[i]];
            const auto v1 = model.vertices[face[(i + 1) % 3]];

            auto start = (glm::dvec2(v0) + 1.0) / 2.0;
            auto end = (glm::dvec2(v1) + 1.0) / 2.0;

            // flip y
            start.y = 1 - start.y;
            end.y = 1 - end.y;

            start *= glm::dvec2(size - 1);
            end *= glm::dvec2(size - 1);

            line(start, end, {1.0, 0.5, 0.0}, buffer, size);
        }
    }

    progress = bufferSize;
}
