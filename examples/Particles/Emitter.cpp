#include <algorithm>

#include <hppv/glad.h>

#include "Emitter.hpp"

void Emitter::update(float frameTime)
{
    frameTime = std::min(frameTime, 0.020f); // useful when debugging

    particles.reserve(life.max * spawn.hz);
    circles.reserve(life.max * spawn.hz);

    for(std::size_t i = 0; i < count; ++i)
    {
        particles[i].life -= frameTime;

        if(particles[i].life <= 0.f)
            killP(i); // note: swap()

        circles[i].center += particles[i].acc * frameTime * frameTime * 0.5f + particles[i].vel * frameTime;
        particles[i].vel += particles[i].acc * frameTime;
        circles[i].color += particles[i].colorVel * frameTime;
    }

    accumulator += frameTime;

    auto spawnDelay = 1.f / spawn.hz;

    while(accumulator > spawnDelay)
    {
        spawnP();
        accumulator -= spawnDelay;
    }
}

void Emitter::render(hppv::Renderer& renderer)
{
    renderer.setBlend(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    renderer.setShader(hppv::Render::CircleColor);
    renderer.cache(circles.data(), count);
}

void Emitter::killP(std::size_t index)
{
    std::swap(particles[index], particles[count - 1]);
    std::swap(circles[index], circles[count - 1]);
    --count;
}

void Emitter::spawnP()
{
    if(count + 1 > particles.size())
    {
        particles.emplace_back();
        circles.emplace_back();
    }

    Particle p;
    hppv::Circle c;

    // center
    {
        Distribution d(0.f, 1.f);
        c.center = spawn.pos + glm::vec2(d(*generator) * spawn.size.x,
                                         d(*generator) * spawn.size.y);
    }
    // radius
    {
        Distribution d(radius.min, radius.max);
        c.radius = d(*generator);
    }
    // life
    {
        Distribution  d(life.min, life.max);
        p.life = d(*generator);
    }
    // color
    {
        Distribution dSR(colorStart.min.r, colorStart.max.r);
        Distribution dSG(colorStart.min.g, colorStart.max.g);
        Distribution dSB(colorStart.min.b, colorStart.max.b);
        Distribution dSA(colorStart.min.a, colorStart.max.a);

        Distribution dER(colorEnd.min.r, colorEnd.max.r);
        Distribution dEG(colorEnd.min.g, colorEnd.max.g);
        Distribution dEB(colorEnd.min.b, colorEnd.max.b);
        Distribution dEA(colorEnd.min.a, colorEnd.min.a);

        c.color = glm::vec4(dSR(*generator), dSG(*generator), dSB(*generator),
                            dSA(*generator));

        p.colorVel = (glm::vec4(dER(*generator), dEG(*generator), dEB(*generator),
                               dEA(*generator)) - c.color) / p.life;
    }
    // vel
    {
        Distribution dX(vel.min.x, vel.max.x);
        Distribution dY(vel.min.y, vel.max.y);
        p.vel = glm::vec2(dX(*generator), dY(*generator));
    }
    // acc
    {
        Distribution dX(acc.min.x, acc.max.x);
        Distribution dY(acc.min.y, acc.max.y);
        p.acc = glm::vec2(dX(*generator), dY(*generator));
    }

    particles[count] = p;
    circles[count] = c;
    ++count;
}
