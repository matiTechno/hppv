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

// -----
// github.com/ssloy/tinyrenderer/wiki

#include <cassert>
#include <utility> // std::swap
#include <vector>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

#include <hppv/Deleter.hpp>

#include "../src/stb_image.h"

void drawLine(glm::ivec2 start, glm::ivec2 end, const glm::dvec3 color,
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

struct Triangle
{
    glm::dvec3 points[3];
    glm::dvec2 texCoords[3];
    glm::dvec3 normals[3];
};

glm::dvec3 barycentric(const glm::dvec3* const points, glm::ivec2 P)
{
    const auto u = glm::cross(glm::dvec3(points[2][0] - points[0][0], points[1][0] - points[0][0], points[0][0] - P[0]),
                              glm::dvec3(points[2][1] - points[0][1], points[1][1] - points[0][1], points[0][1] - P[1]));

    // do we need it? the code seems to work without it
    //if(glm::abs(u[2]) < 1.0)
        //return {-1.0, 1.0, 1.0}; // triangle is degenerate, in this case return smth with negative coordinates

    return {1.0 - (u.x + u.y) / u.z, u.y / u.z, u.x/ u.z};
}

class Texture
{
public:
    Texture(const std::string& filename)
    {
        data_ = stbi_load(filename.c_str(), &size_.x, &size_.y, nullptr, 3);

        if(data_)
        {
            del_.set([data = data_]{stbi_image_free(data);});
        }
        else
        {
            std::cout << "stbi_load() failed: " << filename << std::endl;
        }
    }

    glm::ivec2 getSize() const {return size_;}

    const Pixel* getData() const {return reinterpret_cast<Pixel*>(data_);}

private:
    hppv::Deleter del_;
    glm::ivec2 size_{0, 0};
    unsigned char* data_;
};

Pixel operator*(Pixel p, glm::dvec3 v)
{
    return {static_cast<unsigned char>(p.r * v.r),
            static_cast<unsigned char>(p.g * v.g),
            static_cast<unsigned char>(p.b * v.b)};
}

void drawTriangle(Triangle t, const glm::dvec3 color, Pixel* const buffer, double* const depthBuffer,
                  const glm::ivec2 imageSize, const Texture& texDiffuse)
{
    // back-face culling
    {
        const auto n = glm::normalize(glm::cross(t.points[1] - t.points[0], t.points[2] - t.points[0]));

        if(glm::dot(n, {0.0, 0.0, 1.f}) <= 0.0)
            return;
    }

    // convert to screen space
    for(auto& point: t.points)
    {
        point.y *= -1.0;
        point = (point + 1.0) / 2.0;
        point *= glm::dvec3(imageSize, 1.0);
    }

    glm::ivec2 start = imageSize - 1;
    glm::ivec2 end(0.0);

    // find the bbox (clipping)
    for(auto i = 0; i < 3; ++i)
    {
        for(int j = 0; j < 2; ++j)
        {
            start[j] = glm::max(0, glm::min(start[j], static_cast<int>(t.points[i][j] + 0.5)));
            end[j] = glm::min(imageSize[j] - 1, glm::max(end[j], static_cast<int>(t.points[i][j] + 0.5)));
        }
    }

    for(auto y = start.y; y <= end.y; ++y)
    {
        for(auto x = start.x; x <= end.x; ++x)
        {
            const auto b = barycentric(t.points, {x, y});

            if(b.x < 0.0 || b.y < 0.0 || b.z < 0.0)
                continue;

            const auto idx = y * imageSize.x + x;

            auto z = 0.0;

            for(auto i = 0; i < 3; ++i)
            {
                z += t.points[i][2] * b[i];
            }

            if(z < depthBuffer[idx])
            {
                depthBuffer[idx] = z;

                glm::dvec2 texCoord(0.0);

                for(auto i = 0; i < 3; ++i)
                {
                    texCoord += t.texCoords[i] * b[i];
                }

                // (we have flipped points vertically when converting to screen space)
                texCoord = (texCoord - 1.0) * -1.0;

                const auto texSize = texDiffuse.getSize();
                const glm::ivec2 texel = texCoord * glm::dvec2(texSize - 1);

                buffer[idx] = texDiffuse.getData()[texel.y * texSize.x + texel.x] * color;
            }
        }
    }
}

struct Face
{
    glm::ivec3 points; // maybe positions would be better
    glm::ivec3 texCoords;
    glm::ivec3 normals;
};

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

        // todo: reserving memory

        std::string line;

        while(std::getline(file, line))
        {
            std::stringstream s(line);
            std::string t;
            s >> t;

            if(t == "v" || t == "vn")
            {
                glm::dvec3 v;

                for(auto i = 0; i < 3; ++i)
                {
                    s >> v[i];
                }

                if(t == "v")
                {
                    points.push_back(v);
                }
                else
                {
                    normals.push_back(v);
                }
            }
            else if(t == "vt")
            {
                glm::dvec2 texCoord;

                for(auto i = 0; i < 2; ++i)
                {
                    s >> texCoord[i];
                }

                texCoords.push_back(texCoord);
            }
            else if(t == "f")
            {
                Face face;
                char dummy;

                for(auto i = 0; i < 3; ++i)
                {
                    s >> face.points[i];
                    s >> dummy;
                    s >> face.texCoords[i];
                    s >> dummy;
                    s >> face.normals[i];

                     // in wavefront obj all indices start at 1, not 0

                    --face.points[i];
                    --face.texCoords[i];
                    --face.normals[i];
                }

                faces.push_back(face);
            }
        }
    }

    std::vector<glm::dvec3> points;
    std::vector<glm::dvec2> texCoords;
    std::vector<glm::dvec3> normals;
    std::vector<Face> faces;
};

void renderImage2(Pixel* const buffer, const glm::ivec2 size, std::atomic_int& progress)
{
    const auto bufferSize = size.x * size.y;

    std::vector<double> depthBuffer(bufferSize, std::numeric_limits<double>::infinity());

    const Model model("res/african_head.obj");
    const Texture texDiffuse("res/african_head_diffuse.tga");

    for(const auto& face: model.faces)
    {
        Triangle t;

        for(auto i = 0; i < 3; ++i)
        {
            t.points[i] = model.points[face.points[i]];
            // remove this after we start using projection transformation
            t.points[i].z *= -1.0;

            t.texCoords[i] = model.texCoords[face.texCoords[i]];
            t.normals[i] = model.normals[face.normals[i]];
        }

        const auto n = glm::normalize(glm::cross(t.points[1] - t.points[0], t.points[2] - t.points[0]));
        const glm::dvec3 cameraDir(0.0, 0.0, -1.0);
        const auto intensity = glm::dot(n, -cameraDir);

        drawTriangle(t, glm::clamp(glm::dvec3(1.0, 1.0, 1.0) * intensity, 0.0, 1.0), buffer, depthBuffer.data(),
                     size, texDiffuse);
    }

    progress = bufferSize;
}
