#include <algorithm>

#include <hppv/glad.h>

#include "Emitter.hpp"

void Emitter::reserveMemory()
{
    particles_.reserve(life.max * spawn.hz);
    circles_.reserve(life.max * spawn.hz);
}

void Emitter::update(float frameTime)
{
    frameTime = std::min(frameTime, 0.020f); // useful when debugging

    for(std::size_t i = 0; i < count_; ++i)
    {
        particles_[i].life -= frameTime;

        if(particles_[i].life <= 0.f)
        {
            killParticle(i); // note: swap()
        }

        circles_[i].center += particles_[i].acc * frameTime * frameTime * 0.5f + particles_[i].vel * frameTime;
        particles_[i].vel += particles_[i].acc * frameTime;
        circles_[i].color += particles_[i].colorVel * frameTime;
    }

    accumulator_ += frameTime;

    auto spawnDelay = 1.f / spawn.hz;

    while(accumulator_ > spawnDelay)
    {
        spawnParticle();
        accumulator_ -= spawnDelay;
    }
}

void Emitter::render(hppv::Renderer& renderer)
{
    renderer.setBlend(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    renderer.setShader(hppv::Render::CircleColor);
    renderer.cache(circles_.data(), count_);
}

void Emitter::killParticle(std::size_t index)
{
    std::swap(particles_[index], particles_[count_ - 1]);
    std::swap(circles_[index], circles_[count_ - 1]);
    --count_;
}

void Emitter::spawnParticle()
{
    if(count_ + 1 > particles_.size())
    {
        particles_.emplace_back();
        circles_.emplace_back();
    }

    Particle p;
    hppv::Circle c;

    // center
    {
        Distribution d(0.f, 1.f);
        c.center = spawn.pos + glm::vec2(d(*generator) * spawn.size.x, d(*generator) * spawn.size.y);
    }
    // radius
    {
        Distribution d(radius.min, radius.max);
        c.radius = d(*generator);
    }
    // life
    {
        Distribution d(life.min, life.max);
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

        c.color = glm::vec4(dSR(*generator), dSG(*generator), dSB(*generator), dSA(*generator));

        p.colorVel = (glm::vec4(dER(*generator), dEG(*generator), dEB(*generator), dEA(*generator))
                      - c.color) / p.life;
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

    particles_[count_] = p;
    circles_[count_] = c;
    ++count_;
}
