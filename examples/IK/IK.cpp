#include <hppv/Prototype.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/Space.hpp>
#include <hppv/glad.h>
#include "../run.hpp"

// Geez, I will never again write such a shitty framework.

struct Arm
{
    glm::vec2 pos;
    float length;
    glm::vec4 color;
    float total_angle;
};

#define pi (3.14159265359)

class IK: public hppv::Prototype
{
public:
    IK(): hppv::Prototype({0, 0, 60, 60})
    {
        Arm arm;
        arm.pos = {30, 30};
        arm.length = 10;
        arm.color = {1,0,0,1};
        arms.push_back(arm);
        arm.pos += glm::vec2(0, -arm.length);

        arm.length = 7;
        arm.color = {0,1,0,1};
        arms.push_back(arm);
        arm.pos += glm::vec2(0, -arm.length);

        arm.length = 5;
        arm.color = {0,0,1,1};
        arms.push_back(arm);
        arm.pos += glm::vec2(0, -arm.length);

        // end effector
        arm.color = {1,1,0,1};
        arms.push_back(arm);
    }

    glm::vec2 target_pos;
    std::vector<Arm> arms;

    void prototypeProcessInput(hppv::Pinput input) override
    {
        target_pos = hppv::mapCursor(input.cursorPos, space_.projected, this);
    }

    void prototypeRender(hppv::Renderer& rnd) override
    {
        float max_angle = frame_.time * (2*pi / 6); // dt * angular velocity

        for(Arm& arm: arms)
            arm.total_angle = 0;

        for(int iter = 0; iter < 50; ++iter)
        {
            for(int arm_id = (int)arms.size() - 2; arm_id >= 0; --arm_id)
            {
                Arm& arm = arms[arm_id];
                glm::vec2 effector_pos = arms.back().pos;
                glm::vec2 arm_effector = effector_pos - arm.pos;
                glm::vec2 arm_target = target_pos - arm.pos;
                float effector_angle = atan2(arm_effector.y, arm_effector.x);
                float target_angle = atan2(arm_target.y, arm_target.x);
                float delta_angle = target_angle - effector_angle;
                float abs_delta = fabs(delta_angle);
                // rotate through a shorter path
                if(abs_delta > pi)
                    delta_angle = -(delta_angle / abs_delta) * (2*pi - abs_delta);

                float prev_total = arm.total_angle;
                arm.total_angle += delta_angle;
                float abs_total = fabs(arm.total_angle);

                if(abs_total > max_angle)
                    arm.total_angle = (arm.total_angle / abs_total) * max_angle;

                delta_angle = arm.total_angle - prev_total;

                // update positions of sub arms and the end effector

                float c = cosf(delta_angle);
                float s = sinf(delta_angle);

                for(int child_id = arm_id + 1; child_id < (int)arms.size(); ++child_id)
                {
                    Arm& child = arms[child_id];
                    child.pos -= arm.pos; // translate to the origin of a rotation
                    child.pos = {c * child.pos.x - s * child.pos.y, s * child.pos.x + c * child.pos.y}; // rotate
                    child.pos += arm.pos; // translate back
                }
            }
        }

        rnd.mode(hppv::RenderMode::Instances);
        rnd.shader(hppv::Render::CircleColor);
        {
            hppv::Circle c;
            c.center = target_pos;
            c.radius = 0.5f;
            c.color = {0,1,0,1};
            rnd.cache(c);
        }
        {
            // effector
            hppv::Circle c;
            c.center = arms.back().pos;
            c.color = arms.back().color;
            c.radius = 0.5f;
            rnd.cache(c);
        }

        rnd.mode(hppv::RenderMode::Vertices);
        rnd.shader(hppv::Render::VerticesColor);
        rnd.primitive(GL_LINE_STRIP);

        for(int i = 0; i < (int)arms.size() - 1; ++i)
        {
            Arm& arm = arms[i];
            hppv::Vertex v;
            v.color = arm.color;
            v.pos = arm.pos;
            rnd.cache(v);
            v.pos = arms[i+1].pos;
            rnd.cache(v);
        }
    }
};

RUN(IK)
