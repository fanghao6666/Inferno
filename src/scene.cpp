#include "scene.h"
#include <json.hpp>
#include <gtc/matrix_transform.hpp>
#include "material.h"
#include "macros.h"
#include "utility.h"

namespace nimble
{
// -----------------------------------------------------------------------------------------------------------------------------------

Scene::Scene(const std::string& name) :
    m_name(name)
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

Scene::~Scene()
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

Entity::ID Scene::create_entity(const std::string& name)
{
    Entity::ID id = m_entities.add();

    Entity& e = m_entities.lookup(id);

    e.id   = id;
    e.name = name;

    return id;
}

// -----------------------------------------------------------------------------------------------------------------------------------

Entity::ID Scene::lookup_entity_id(const std::string& name)
{
    for (uint32_t i = 0; i < m_entities.size(); i++)
    {
        if (m_entities._objects[i].name == name)
            return m_entities._objects[i].id;
    }

    return USHRT_MAX;
}

// -----------------------------------------------------------------------------------------------------------------------------------

Entity& Scene::lookup_entity(const std::string& name)
{
    Entity::ID id = lookup_entity_id(name);
    return m_entities.lookup(id);
}

// -----------------------------------------------------------------------------------------------------------------------------------

Entity& Scene::lookup_entity(const Entity::ID& id)
{
    return m_entities.lookup(id);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Scene::update_entity(Entity e)
{
    Entity& old_entity = lookup_entity(e.id);
    old_entity         = e;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Scene::destroy_entity(const Entity::ID& id)
{
    if (m_entities.has(id))
        m_entities.remove(id);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Scene::destroy_entity(const std::string& name)
{
    Entity::ID id = lookup_entity_id(name);

    if (id != USHRT_MAX && m_entities.has(id))
    {
        Entity& e = lookup_entity(id);
        e.~Entity();

        m_entities.remove(id);
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

AABB Scene::aabb()
{
    AABB out = { glm::vec3(FLT_MAX), glm::vec3(-FLT_MAX) };

    for (uint32_t i = 0; i < m_entities.size(); i++)
    {
        AABB b = m_entities._objects[i].mesh->aabb();

        glm::vec4 min = m_entities._objects[i].transform.model * glm::vec4(b.min, 0.0f);
        glm::vec4 max = m_entities._objects[i].transform.model * glm::vec4(b.max, 0.0f);

        if (min.x < out.min.x)
            out.min.x = min.x;
        if (min.y < out.min.y)
            out.min.y = min.y;
        if (min.z < out.min.z)
            out.min.z = min.z;

        if (max.x > out.max.x)
            out.max.x = max.x;
        if (max.y > out.max.y)
            out.max.y = max.y;
        if (max.z > out.max.z)
            out.max.z = max.z;
    }

    return out;
}

// -----------------------------------------------------------------------------------------------------------------------------------

ReflectionProbe::ID Scene::create_reflection_probe(const glm::vec3& position, const glm::vec3& extents)
{
    ReflectionProbe::ID id = m_reflection_probes.add();

    ReflectionProbe& p = m_reflection_probes.lookup(id);

    p.id       = id;
    p.position = position;
    p.extents  = extents;

    return id;
}

// -----------------------------------------------------------------------------------------------------------------------------------

GIProbe::ID Scene::create_gi_probe(const glm::vec3& position)
{
    GIProbe::ID id = m_gi_probes.add();

    GIProbe& p = m_gi_probes.lookup(id);

    p.id       = id;
    p.position = position;

    return id;
}

// -----------------------------------------------------------------------------------------------------------------------------------

ReflectionProbe& Scene::lookup_reflection_probe(const ReflectionProbe::ID& id)
{
    return m_reflection_probes.lookup(id);
}

// -----------------------------------------------------------------------------------------------------------------------------------

GIProbe& Scene::lookup_gi_probe(const GIProbe::ID& id)
{
    return m_gi_probes.lookup(id);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Scene::destroy_reflection_probe(const ReflectionProbe::ID& id)
{
    if (m_reflection_probes.has(id))
        m_reflection_probes.remove(id);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Scene::destroy_gi_probe(const GIProbe::ID& id)
{
    if (m_gi_probes.has(id))
        m_gi_probes.remove(id);
}

// -----------------------------------------------------------------------------------------------------------------------------------

PointLight::ID Scene::create_point_light(const glm::vec3& position, const glm::vec3& color, const float& range, const float& intensity, const bool& casts_shadows, const float& shadow_map_bias)
{
    PointLight::ID id = m_point_lights.add();

    PointLight& p = m_point_lights.lookup(id);

    p.id                 = id;
    p.transform.position = position;
    p.color              = color;
    p.range              = range;
    p.intensity          = intensity;
    p.enabled            = true;
    p.casts_shadow       = casts_shadows;
    p.shadow_map_bias    = shadow_map_bias;
    p.transform.update();

    return id;
}

// -----------------------------------------------------------------------------------------------------------------------------------

PointLight& Scene::lookup_point_light(const PointLight::ID& id)
{
    return m_point_lights.lookup(id);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Scene::destroy_point_light(const PointLight::ID& id)
{
    m_point_lights.remove(id);
}

// -----------------------------------------------------------------------------------------------------------------------------------

SpotLight::ID Scene::create_spot_light(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& color, const float& inner_cone_angle, const float& outer_cone_angle, const float& range, const float& intensity, const bool& casts_shadows, const float& shadow_map_bias)
{
    SpotLight::ID id = m_spot_lights.add();

    SpotLight& p = m_spot_lights.lookup(id);

    p.id                 = id;
    p.transform.position = position;
    p.color              = color;
    p.range              = range;
    p.intensity          = intensity;
    p.inner_cone_angle   = inner_cone_angle;
    p.outer_cone_angle   = outer_cone_angle;
    p.enabled            = true;
    p.casts_shadow       = casts_shadows;
    p.shadow_map_bias    = shadow_map_bias;
    p.transform.set_orientation_from_euler_yxz(rotation);
    p.transform.update();

    return id;
}

// -----------------------------------------------------------------------------------------------------------------------------------

SpotLight& Scene::lookup_spot_light(const SpotLight::ID& id)
{
    return m_spot_lights.lookup(id);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Scene::destroy_spot_light(const SpotLight::ID& id)
{
    m_spot_lights.remove(id);
}

// -----------------------------------------------------------------------------------------------------------------------------------

DirectionalLight::ID Scene::create_directional_light(const glm::vec3& rotation, const glm::vec3& color, const float& intensity, const bool& casts_shadows)
{
    DirectionalLight::ID id = m_directional_lights.add();

    DirectionalLight& p = m_directional_lights.lookup(id);

    p.id           = id;
    p.color        = color;
    p.intensity    = intensity;
    p.enabled      = true;
    p.casts_shadow = casts_shadows;
    p.transform.set_orientation_from_euler_yxz(rotation);
    p.transform.update();

    return id;
}

// -----------------------------------------------------------------------------------------------------------------------------------

DirectionalLight& Scene::lookup_directional_light(const DirectionalLight::ID& id)
{
    return m_directional_lights.lookup(id);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Scene::destroy_directional_light(const DirectionalLight::ID& id)
{
    m_directional_lights.remove(id);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Scene::update_reflection_probes()
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Scene::update_gi_probes()
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Scene::update()
{
    for (uint32_t i = 0; i < m_entities.size(); i++)
    {
        Entity& e = m_entities._objects[i];

        if (e.dirty)
            e.transform.update();
    }

    for (uint32_t i = 0; i < m_directional_lights.size(); i++)
        m_directional_lights._objects[i].transform.update();

    for (uint32_t i = 0; i < m_spot_lights.size(); i++)
        m_spot_lights._objects[i].transform.update();

    for (uint32_t i = 0; i < m_point_lights.size(); i++)
        m_point_lights._objects[i].transform.update();
}

// -----------------------------------------------------------------------------------------------------------------------------------
} // namespace nimble