#include <vector>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <random>

#include <hppv/Prototype.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/glad.h>
#include <hppv/imgui.h>

#include <GLFW/glfw3.h>

#include "../run.hpp"

glm::vec2 get_spline_point(std::vector<glm::vec2> control_points, float t)
{
    assert(control_points.size() >= 4);
    int id1 = (int)t;
    assert(id1 < control_points.size());

    int id0 = id1 - 1;

    if(id0 < 0)
        id0 = control_points.size() - 1;

    int id2 = id1 + 1;

    if(id2 == control_points.size())
        id2 = 0;

    int id3 = id2 + 1;

    if(id3 == control_points.size())
        id3 = 0;

    glm::vec2 p0 = control_points[id0];
    glm::vec2 p1 = control_points[id1];
    glm::vec2 p2 = control_points[id2];
    glm::vec2 p3 = control_points[id3];

    t = t - (int)t; // cut the whole part

    float tt = t * t;
    float ttt = t * t * t;

    // 4 basis functions

    float b0 = -ttt + 2.f * tt - t;
    float b1 = 3.f * ttt - 5.f * tt + 2.f;
    float b2 = -3.f * ttt + 4.f * tt + t;
    float b3 = ttt - tt;

    return 0.5f * (p0 * b0 + p1 * b1 + p2 * b2 + p3 * b3);
}

glm::vec2 spline_derivative(std::vector<glm::vec2> control_points, float t)
{
    assert(control_points.size() >= 4);
    int id1 = (int)t;
    assert(id1 < control_points.size());

    int id0 = id1 - 1;

    if(id0 < 0)
        id0 = control_points.size() - 1;

    int id2 = id1 + 1;

    if(id2 == control_points.size())
        id2 = 0;

    int id3 = id2 + 1;

    if(id3 == control_points.size())
        id3 = 0;

    glm::vec2 p0 = control_points[id0];
    glm::vec2 p1 = control_points[id1];
    glm::vec2 p2 = control_points[id2];
    glm::vec2 p3 = control_points[id3];

    t = t - (int)t; // cut the whole part

    float tt = t * t;

    float b0 = -3.f * tt + 4.f * t - 1.f;
    float b1 = 9.f * tt - 10.f * t;
    float b2 = -9.f * tt + 8.f * t + 1.f;
    float b3 = 3.f * tt - 2.f * t;

    return 0.5f * (p0 * b0 + p1 * b1 + p2 * b2 + p3 * b3);
}

glm::vec2 segments_intersection_point(bool* valid, glm::vec2 begin1, glm::vec2 end1, glm::vec2 begin2, glm::vec2 end2)
{
    // todo?, vertical lines, maybe swap x with y under some threshold
    // I hope this will not interfere with a neural net
    assert(end1.x - begin1.x);
    assert(end2.x - begin2.x);

    *valid = false;
    float a1 =  (end1.y - begin1.y) / (end1.x - begin1.x);
    float b1 = begin1.y - a1 * begin1.x;
    float a2 =  (end2.y - begin2.y) / (end2.x - begin2.x);
    float b2 = begin2.y - a2 * begin2.x;
    float x = (b2 - b1) / (a1 - a2);
    float y = a1 * x + b1;
    glm::vec2 ipoint = {x, y};
    glm::vec2 d1 = end1 - begin1;
    glm::vec2 d2 = end2 - begin2;
    glm::vec2 d1_ipoint = ipoint - begin1;
    glm::vec2 d2_ipoint = ipoint - begin2;

    if(length(d1_ipoint) > length(d1) || dot(d1, d1_ipoint) < 0)
        return {};

    if(length(d2_ipoint) > length(d2) || dot(d2, d2_ipoint) < 0)
        return {};

    *valid = true;
    return ipoint;
}

struct Net
{
    int in_size;
    int hid_size;
    int out_size;

    float* out_a;
    float* hid_a;
    float* out_biases;
    float* hid_biases;
    float* oh_weights;
    float* hi_weights;
};

struct Sensor
{
    float angle;
    glm::vec2 ipoint; // intersection point, cached query result
    float range;
};

struct Car
{
    Net net;
    glm::vec2 pos; // front-center
    glm::vec2 dir;
    float speed;
    Sensor sensors[5];
    bool eliminated;
    int checkpoint_id;
    float score;
    float timer;
    // controls
    bool accelerate;
    bool break_;
    bool turn_left;
    bool turn_right;

    struct
    {
        glm::vec2 front_left;
        glm::vec2 front_right;
        glm::vec2 back_left;
        glm::vec2 back_right;
    } v; // vertices of a body
};

// dir is  normalized
// note: y grows down so the rotation direction is flipped

glm::vec2 rotate_dir(glm::vec2 dir, float angle)
{
    angle += atan2f(dir.y, dir.x); // add delta angle to a previous angle
    return {cosf(angle), sinf(angle)};
}

#define CAR_LENGHT 2.3f

void gen_car_vertices(Car& car)
{
    float width = 0.6f;
    float length = CAR_LENGHT;
    glm::vec2 normal = rotate_dir(car.dir, 3.14 / 2.f);
    car.v.front_left = car.pos + normal * width;
    car.v.front_right = car.pos - normal * width;
    car.v.back_left = car.v.front_left - car.dir * length;
    car.v.back_right = car.v.front_right - car.dir * length;
}

void query_sensor(Sensor& sensor, Car& car, std::vector<glm::vec2>& border_points)
{
    glm::vec2 sdir = rotate_dir(car.dir, sensor.angle);
    float min_dist = length(sensor.ipoint - car.pos);

    for(int i = 0; i < border_points.size(); ++i)
    {
        glm::vec2 v1 = border_points[i];
        int idx2 = i + 1 == border_points.size() ? 0 : i + 1;
        glm::vec2 v2 = border_points[idx2];
        bool valid;
        glm::vec2 ipoint = segments_intersection_point(&valid, car.pos, car.pos + sdir * 1000.f, v1, v2);

        if(!valid)
            continue;

        float dist = glm::length(ipoint - car.pos);

        if(dist < min_dist)
        {
            min_dist = dist;
            sensor.ipoint = ipoint;
        }
    }
}

bool query_car_collision(Car& car, std::vector<glm::vec2>& border_points)
{
    for(int i = 0; i < border_points.size(); ++i)
    {
        glm::vec2 v1 = border_points[i];
        int idx2 = i + 1 == border_points.size() ? 0 : i + 1;
        glm::vec2 v2 = border_points[idx2];

        bool valid;
        segments_intersection_point(&valid, v1, v2, car.v.back_left, car.v.front_left);

        if(valid)
            return true;

        segments_intersection_point(&valid, v1, v2, car.v.back_right, car.v.front_right);

        if(valid)
            return true;

        // todo? front and back segments, not only sides
    }
    return false;
}

std::mt19937 _rng;

float get_random01()
{
    std::uniform_real_distribution<float> d(0.f, 1.f);
    return d(_rng);
}

void net_randomize_params(Net& net)
{
    for(int i = 0; i < net.out_size; ++i)
    {
        net.out_biases[i] = get_random01() * 2.f - 1.f;

        for(int k = 0; k < net.hid_size; ++k)
            net.oh_weights[i * net.hid_size + k] = get_random01() * 2.f - 1.f;
    }

    for(int i = 0; i < net.hid_size; ++i)
    {
        net.hid_biases[i] = get_random01() * 2.f - 1.f;

        for(int k = 0; k < net.in_size; ++k)
            net.hi_weights[i * net.in_size + k] = get_random01() * 2.f - 1.f;
    }
}

void net_init(Net& net, int in_size, int hid_size, int out_size)
{
    net.in_size = in_size;
    net.hid_size = hid_size;
    net.out_size = out_size;

    int fs = sizeof(float);

    net.out_a = (float*)malloc(out_size * fs);
    net.hid_a = (float*)malloc(hid_size * fs);
    net.out_biases = (float*)malloc(out_size * fs);
    net.hid_biases = (float*)malloc(hid_size * fs);
    net.oh_weights = (float*)malloc(out_size * hid_size * fs);
    net.hi_weights = (float*)malloc(hid_size * in_size * fs);

    net_randomize_params(net);
}

void restart_car(Car& car, std::vector<glm::vec2>& checkpoints)
{
    car.pos = checkpoints[0];
    car.dir = glm::normalize(checkpoints[1] - checkpoints[0]);
    car.speed = 0.f;
    car.timer = 0.f;
    car.eliminated = false;
    car.checkpoint_id = 0;
    car.score = 0.f;
    assert(sizeof(car.sensors) / sizeof(Sensor) == 5);
    car.sensors[0].angle = 0.f;
    car.sensors[0].range = 60.f;

    for(int i = 1; i < 5; ++i)
        car.sensors[i].range = 7.f;

    car.sensors[1].angle = 3.14f / 6.f;
    car.sensors[2].angle = -3.14f / 6.f;
    car.sensors[3].angle = 3.14f / 3.f;
    car.sensors[4].angle = -3.14f / 3.f;
    gen_car_vertices(car);
    car.accelerate = false;
    car.break_ = false;
    car.turn_left = false;
    car.turn_right = false;
}

float sigmoid(int x)
{
    return 1.f / (1.f + expf(-x));
}

void net_fprop(Net& net, float* input)
{
    for(int i = 0; i < net.hid_size; ++i)
    {
        float z = net.hid_biases[i];

        for(int k = 0; k < net.in_size; ++k)
            z += net.hi_weights[i * net.in_size + k] * input[k];

        net.hid_a[i] = sigmoid(z);
    }

    for(int i = 0; i < net.out_size; ++i)
    {
        float z = net.out_biases[i];

        for(int k = 0; k < net.hid_size; ++k)
            z += net.oh_weights[i * net.hid_size + k] * net.hid_a[k];

        net.out_a[i] = sigmoid(z);
    }
}

void net_mutate(Net& net)
{
    for(int i = 0; i < net.out_size; ++i)
    {
        float x = get_random01();

        if(x < 0.3f)
            net.out_biases[i] += (get_random01() * 2.f - 1.f);
    }

    for(int i = 0; i < net.hid_size; ++i)
    {
        float x = get_random01();

        if(x < 0.3f)
            net.hid_biases[i] += (get_random01() * 2.f - 1.f);
    }

    for(int i = 0; i < net.out_size * net.hid_size; ++i)
    {
        float x = get_random01();

        if(x < 0.3f)
            net.oh_weights[i] += (get_random01() * 2.f - 1.f);
    }

    for(int i = 0; i < net.hid_size * net.in_size; ++i)
    {
        float x = get_random01();

        if(x < 0.3f)
            net.hi_weights[i] += (get_random01() * 2.f - 1.f);
    }
}

void car_compute(Car& car)
{
    assert(car.net.in_size == 6);
    float input[6];
    assert(sizeof(car.sensors) / sizeof(Sensor) == 5);

    for(int i = 0; i < 5; ++i)
    {
        Sensor& sens = car.sensors[i];
        float dist = glm::length(sens.ipoint - car.pos);
        input[i] = (sens.range - dist) / sens.range;
    }

    input[5] = car.speed;

    net_fprop(car.net, input);

    car.accelerate = car.net.out_a[0] > 0.5f;;
    car.break_ = car.net.out_a[1] > 0.5f;
    car.turn_left = car.net.out_a[2] > 0.5f;
    car.turn_right = car.net.out_a[3] > 0.5f;
}

#define MAX_CP_TIME 3.f

class NeuroCar: public hppv::Prototype
{
public:
    std::vector<glm::vec2> checkpoints;
    std::vector<glm::vec2> border1;
    std::vector<glm::vec2> border2;
    std::vector<Car> cars;
    int speedup = 1;
    float best_prev_score = 0.f;
    int generation = 0;
    bool human_player = false;
    float accumulator = 0.f;

    NeuroCar():
        hppv::Prototype({0.f, 0.f, 100.f, 100.f})
    {
        std::random_device rd;
        _rng.seed(rd());

        std::vector<glm::vec2> control_points = {{10.f, 10.f}, {-20.f, 40.f}, {30.f, 40.f}, {60.f, 60.f}, {60.f, 10.f}};

        for(float t = 0.f; t < (float)control_points.size(); t += 0.1f)
        {
            glm::vec2 p = get_spline_point(control_points, t);
            glm::vec2 tanv = spline_derivative(control_points, t); // vector tangent to the spline
            tanv = normalize(tanv);
            glm::vec2 normal = rotate_dir(tanv, 3.14f / 2.f); // rotate by 90 degrees
            glm::vec2 p1 = p + normal * 3.f;
            glm::vec2 p2 = p - normal * 3.f;
            checkpoints.push_back(p);
            border1.push_back(p1);
            border2.push_back(p2);
        }

        for(int i = 0; i < 100; ++i)
        {
            Car car;
            restart_car(car, checkpoints);
            // inputs: 5 sensor inputs and speed
            // outputs: accelerate, break, turn left, turn right
            assert(sizeof(car.sensors) / sizeof(Sensor) == 5);
            net_init(car.net, 6, 12, 4);
            cars.push_back(car);
        }
    }

    void prototypeProcessInput(hppv::Pinput in) override
    {
        assert(cars.size());

        for(const hppv::Event& e: in.events)
        {
            if(e.type == hppv::Event::Key && e.key.action != GLFW_REPEAT)
            {
                switch(e.key.key)
                {
                case GLFW_KEY_W:
                    cars[0].accelerate = e.key.action == GLFW_PRESS;
                    break;
                case GLFW_KEY_S:
                    cars[0].break_ = e.key.action == GLFW_PRESS;
                    break;
                case GLFW_KEY_A:
                    cars[0].turn_left = e.key.action == GLFW_PRESS;
                    break;
                case GLFW_KEY_D:
                    cars[0].turn_right = e.key.action == GLFW_PRESS;
                    break;
                }
            }
        }
    }

    void sim(float dt)
    {
        for(Car& car: cars)
        {
            if(car.eliminated)
                continue;

            for(Sensor& sensor: car.sensors)
            {
                sensor.ipoint = car.pos + rotate_dir(car.dir, sensor.angle) * sensor.range;
                query_sensor(sensor, car, border1);
                query_sensor(sensor, car, border2);
            }

            if(!human_player)
                car_compute(car);

            float angular_vel = 3.14f / 2.f;
            float drag = 1.f;
            float turn_dir = car.turn_right - car.turn_left;
            float acc = (car.accelerate - car.break_) * 10.f;

            car.pos += car.speed * car.dir * dt;
            car.speed += (acc - drag) * dt;
            car.dir = rotate_dir(car.dir, turn_dir * angular_vel * dt);

            if(car.speed < 0.05f && acc <= 0.f)
                car.speed = 0.f;

            gen_car_vertices(car);

            bool collision = query_car_collision(car, border1) || query_car_collision(car, border2);

            if(collision)
                car.eliminated = true;

            if(human_player)
                continue;

            car.timer += dt;

            if(car.timer > MAX_CP_TIME)
                car.eliminated = true;

            int prev_cp_id = car.checkpoint_id == 0 ? checkpoints.size() - 1 : car.checkpoint_id - 1;
            int next_cp_id = car.checkpoint_id + 1 == checkpoints.size() ? 0 : car.checkpoint_id + 1;

            float dist_cp_current = glm::length(checkpoints[car.checkpoint_id] - car.pos);
            float dist_cp_prev = glm::length(checkpoints[prev_cp_id] - car.pos);
            float dist_cp_next = glm::length(checkpoints[next_cp_id] - car.pos);

            if(dist_cp_prev < dist_cp_current)
                car.eliminated = true;

            if(dist_cp_next < dist_cp_current)
            {
                car.score += (MAX_CP_TIME - car.timer) / MAX_CP_TIME;
                car.checkpoint_id = next_cp_id;
                car.timer = 0.f;
            }

            if(car.score && car.checkpoint_id == 0) // restart after one lap
            {
                for(Car& car: cars)
                    car.eliminated = true;
            }
        }

        bool next_gen = true;

        for(Car& car: cars)
        {
            if(!car.eliminated)
            {
                next_gen = false;
                break;
            }
        }

        if(next_gen)
        {
            if(human_player)
            {
                restart_car(cars[0], checkpoints);
                return;
            }

            assert(cars.size());
            generation += 1;
            Car* best = &cars[0];

            for(Car& car: cars)
            {
                if(car.score > best->score)
                    best = &car;
            }

            best_prev_score = best->score;

            for(Car& car: cars)
            {
                if(&car == best)
                    continue;

                memcpy(car.net.out_biases, best->net.out_biases, sizeof(float) * car.net.out_size);
                memcpy(car.net.hid_biases, best->net.hid_biases, sizeof(float) * car.net.hid_size);
                memcpy(car.net.oh_weights, best->net.oh_weights, sizeof(float) * car.net.out_size * car.net.hid_size);
                memcpy(car.net.hi_weights, best->net.hi_weights, sizeof(float) * car.net.hid_size * car.net.in_size);

                net_mutate(car.net);
            }

            for(Car& car: cars)
                restart_car(car, checkpoints);
        }
    }

    void prototypeRender(hppv::Renderer& rr) override
    {
        float dt = 0.007f;
        accumulator += frame_.time * speedup;

        if(accumulator > 100 * dt) // avoid the spiral of death
            accumulator = 100 * dt;

        while(accumulator >= dt)
        {
            accumulator -= dt;
            sim(dt);
        }

        ImGui::Begin(prototype_.imguiWindowName);
        ImGui::Text("generation: %d", generation);
        ImGui::Text("best prev score: %f", best_prev_score);
        ImGui::InputInt("speedup", &speedup);

        if(ImGui::Checkbox("human player", &human_player))
        {
            for(Car& car: cars)
                car.eliminated = true;
        }

        if(ImGui::Button("restart"))
        {
            for(Car& car: cars)
            {
                net_randomize_params(car.net);

                if(!human_player)
                    restart_car(car, checkpoints);
            }
            generation = 0;
        }

        ImGui::End();

        rr.mode(hppv::RenderMode::Vertices);
        rr.shader(hppv::Render::VerticesColor);
        rr.primitive(GL_LINE_LOOP);

        for(glm::vec2 p: border1)
        {
            hppv::Vertex v;
            v.pos = p;
            rr.cache(v);
        }

        rr.breakBatch();

        for(glm::vec2 p: border2)
        {
            hppv::Vertex v;
            v.pos = p;
            rr.cache(v);
        }

        rr.mode(hppv::RenderMode::Instances);
        rr.shader(hppv::Render::CircleColor);

        for(glm::vec2 p: checkpoints)
        {
            hppv::Circle c;
            c.radius = 0.25f;
            c.color *= 0.5f;
            c.center = p;
            rr.cache(c);
        }

        rr.mode(hppv::RenderMode::Vertices);
        rr.shader(hppv::Render::VerticesColor);
        rr.primitive(GL_LINE_LOOP);

        for(Car& car: cars)
        {
            if(car.eliminated)
                continue;

            hppv::Vertex verts[4];
            verts[0].pos = car.v.front_left;
            verts[1].pos = car.v.front_right;
            verts[2].pos = car.v.back_right;
            verts[3].pos = car.v.back_left;
            rr.cache(verts, 4);
            rr.breakBatch();
            verts[0].color = verts[1].color = {1.f, 1.f, 0.f, 1.f};

            for(Sensor& sensor: car.sensors)
            {
                verts[0].pos = car.pos;
                verts[1].pos = sensor.ipoint;
                rr.cache(verts, 2);
                rr.breakBatch();
            }
        }
    }
};

RUN(NeuroCar)
