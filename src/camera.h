#pragma once

#include <glm.hpp>
#include <gtc/quaternion.hpp>
#include <vector>
#include <memory>
#include "geometry.h"

namespace nimble
{
struct Viewport;

struct Camera
{
    float     m_fov;
    float     m_near;
    float     m_far;
    float     m_aspect_ratio;
    glm::vec3 m_position;
    glm::vec3 m_forward;
    glm::vec3 m_up;
    glm::vec3 m_right;
    glm::vec3 m_world_up;
    glm::quat m_orientation;
    float     m_yaw;
    float     m_pitch;
    float     m_roll;

    glm::mat4 m_view;
    glm::mat3 m_model;
    glm::mat4 m_projection;
    glm::mat4 m_raw_projection;
    glm::mat4 m_view_projection;
    glm::mat4 m_prev_view_projection;
    glm::mat4 m_rotate;
    glm::mat4 m_translate;

    float m_near_begin = 0.0f;
    float m_near_end   = 0.0f;
    float m_far_begin  = 0.0f;
    float m_far_end    = 0.0f;

    Plane m_near_plane;
    Plane m_far_plane;
    Plane m_left_plane;
    Plane m_right_plane;
    Plane m_top_plane;
    Plane m_bottom_plane;

    Frustum m_frustum;

    uint32_t                  m_index;
    std::vector<glm::vec2>    m_jitter_samples;
    bool                      m_half_pixel_jitter;
    glm::vec2                 m_current_jitter;
    glm::vec2                 m_prev_jitter;
    uint32_t                  m_width;
    uint32_t                  m_height;
    std::shared_ptr<Viewport> m_viewport = nullptr;

    Camera(float fov, float near, float far, float aspect_ratio, glm::vec3 position, glm::vec3 forward);
    void reset(float fov, float near, float far, float aspect_ratio, glm::vec3 position, glm::vec3 forward);
    void set_translation_delta(glm::vec3 direction, float amount);
    void set_rotatation_delta(glm::vec3 angles);
    void set_position(glm::vec3 position);
    void update();
    void update_projection(float fov, float near, float far, float aspect_ratio);
    bool aabb_inside_frustum(glm::vec3 max_v, glm::vec3 min_v);
    bool aabb_inside_plane(Plane plane, glm::vec3 max_v, glm::vec3 min_v);
    void apply_jitter();
};
} // namespace nimble