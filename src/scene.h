#pragma once

#include "entity.h"
#include "packed_array.h"
#include "camera.h"
#include "lights.h"
#include "constants.h"
#include <vector>
#include <string>

namespace nimble
{
#define ENVIRONMENT_MAP_SIZE 1024

struct ReflectionProbe
{
    using ID = uint32_t;

    ID        id;
    glm::vec3 extents;
    glm::vec3 position;
};

struct GIProbe
{
    using ID = uint32_t;

    ID        id;
    glm::vec3 position;
};

class Scene
{
public:
    Scene(const std::string& name);
    ~Scene();

    // Updates entity transforms.
    void update();
    void update_reflection_probes();
    void update_gi_probes();

    // Entity manipulation methods.
    Entity::ID create_entity(const std::string& name);
    Entity::ID lookup_entity_id(const std::string& name);
    Entity&    lookup_entity(const std::string& name);
    Entity&    lookup_entity(const Entity::ID& id);
    void       update_entity(Entity e);
    void       destroy_entity(const Entity::ID& id);
    void       destroy_entity(const std::string& name);

    AABB aabb();

    // Probe manipulation methods.
    ReflectionProbe::ID create_reflection_probe(const glm::vec3& position, const glm::vec3& extents);
    ReflectionProbe&    lookup_reflection_probe(const ReflectionProbe::ID& id);
    void                destroy_reflection_probe(const ReflectionProbe::ID& id);
    GIProbe::ID         create_gi_probe(const glm::vec3& position);
    GIProbe&            lookup_gi_probe(const GIProbe::ID& id);
    void                destroy_gi_probe(const GIProbe::ID& id);

    // Light manipulation methods.
    PointLight::ID       create_point_light(const glm::vec3& position, const glm::vec3& color, const float& range, const float& intensity, const bool& casts_shadows = false, const float& shadow_map_bias = 0.0f);
    PointLight&          lookup_point_light(const PointLight::ID& id);
    void                 destroy_point_light(const PointLight::ID& id);
    SpotLight::ID        create_spot_light(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& color, const float& inner_cone_angle, const float& outer_cone_angle, const float& range, const float& intensity, const bool& casts_shadows = false, const float& shadow_map_bias = 0.05f);
    SpotLight&           lookup_spot_light(const SpotLight::ID& id);
    void                 destroy_spot_light(const SpotLight::ID& id);
    DirectionalLight::ID create_directional_light(const glm::vec3& rotation, const glm::vec3& color, const float& intensity, const bool& casts_shadows = false);
    DirectionalLight&    lookup_directional_light(const DirectionalLight::ID& id);
    void                 destroy_directional_light(const DirectionalLight::ID& id);

    // Inline setters.
    inline void set_camera(std::shared_ptr<Camera> camera) { m_camera = camera; }
    inline void set_name(const std::string& name) { m_name = name; }
    inline void set_environment_map(const std::shared_ptr<TextureCube>& texture) { m_env_map = texture; }
    inline void set_irradiance_map(const std::shared_ptr<TextureCube>& texture) { m_irradiance_map = texture; }
    inline void set_prefiltered_map(const std::shared_ptr<TextureCube>& texture) { m_prefiltered_map = texture; }

    // Inline getters.
    inline std::shared_ptr<Camera>       camera() { return m_camera; }
    inline uint32_t                      entity_count() { return m_entities.size(); }
    inline Entity*                       entities() { return &m_entities._objects[0]; }
    inline uint32_t                      reflection_probe_count() { return m_reflection_probes.size(); }
    inline ReflectionProbe*              reflection_probes() { return &m_reflection_probes._objects[0]; }
    inline uint32_t                      gi_probe_count() { return m_gi_probes.size(); }
    inline GIProbe*                      gi_probes() { return &m_gi_probes._objects[0]; }
    inline uint32_t                      point_light_count() { return m_point_lights.size(); }
    inline PointLight*                   point_lights() { return &m_point_lights._objects[0]; }
    inline uint32_t                      spot_light_count() { return m_spot_lights.size(); }
    inline SpotLight*                    spot_lights() { return &m_spot_lights._objects[0]; }
    inline uint32_t                      directional_light_count() { return m_directional_lights.size(); }
    inline DirectionalLight*             directional_lights() { return &m_directional_lights._objects[0]; }
    inline std::string                   name() const { return m_name; }
    inline std::shared_ptr<TextureCube>& env_map() { return m_env_map; }
    inline std::shared_ptr<TextureCube>& irradiance_map() { return m_irradiance_map; }
    inline std::shared_ptr<TextureCube>& prefiltered_map() { return m_prefiltered_map; }
    inline std::shared_ptr<TextureCube>& reflection_probe_cubemap() { return m_reflection_probe_cubemap; }
    inline std::shared_ptr<TextureCube>& gi_probe_cubemap() { return m_gi_probe_cubemap; }

private:
    std::string                                           m_name;
    std::shared_ptr<Camera>                               m_camera;
    PackedArray<ReflectionProbe, MAX_RELFECTION_PROBES>   m_reflection_probes;
    PackedArray<GIProbe, MAX_GI_PROBES>                   m_gi_probes;
    PackedArray<Entity, MAX_ENTITIES>                     m_entities;
    PackedArray<PointLight, MAX_POINT_LIGHTS>             m_point_lights;
    PackedArray<SpotLight, MAX_SPOT_LIGHTS>               m_spot_lights;
    PackedArray<DirectionalLight, MAX_DIRECTIONAL_LIGHTS> m_directional_lights;
    // PBR cubemaps common to the entire scene.
    std::shared_ptr<TextureCube> m_env_map;
    std::shared_ptr<TextureCube> m_irradiance_map;
    std::shared_ptr<TextureCube> m_prefiltered_map;
    // Probe cubemap arrays
    std::shared_ptr<TextureCube> m_reflection_probe_cubemap;
    std::shared_ptr<TextureCube> m_gi_probe_cubemap;
};
} // namespace nimble