#pragma once

#include "geometry.h"
#include "mesh.h"
#include "material.h"
#include "ogl.h"
#include "transform.h"
#include <string>

#define ENABLE_SUBMESH_CULLING

namespace nimble
{
struct Entity
{
    using ID = uint32_t;

    ID                        id;
    std::string               name;
    std::shared_ptr<Material> override_mat;
    std::shared_ptr<Mesh>     mesh;
    OBB                       obb;
    uint64_t                  visibility_flags;
    bool                      dirty;
    bool                      is_static;
    Transform                 transform;

#ifdef ENABLE_SUBMESH_CULLING
    std::vector<Sphere>   submesh_spheres;
    std::vector<uint64_t> submesh_visibility_flags;
#endif

    Entity()
    {
        is_static = false;
        dirty     = true;
    }

    inline void set_position(const glm::vec3& p)
    {
        transform.position = p;
        dirty              = true;
    }
    inline void set_rotation(const glm::vec3& r)
    {
        transform.set_orientation_from_euler_xyz(r);
        dirty = true;
    }
    inline void set_scale(const glm::vec3& s)
    {
        transform.scale = s;
        dirty           = true;
    }
    inline bool visibility(const uint32_t& view_index) { return (visibility_flags & BIT_FLAG_64(view_index)) == BIT_FLAG_64(view_index); }
    inline void set_visible(const uint32_t& view_index) { SET_BIT_64(visibility_flags, view_index); }
    inline void set_invisible(const uint32_t& view_index) { CLEAR_BIT_64(visibility_flags, view_index); }

#ifdef ENABLE_SUBMESH_CULLING
    inline bool submesh_visibility(const uint32_t& submesh_index, const uint32_t& view_index)
    {
        return (submesh_visibility_flags[submesh_index] & BIT_FLAG_64(view_index)) == BIT_FLAG_64(view_index);
    }
    inline void set_submesh_visible(const uint32_t& submesh_index, const uint32_t& view_index) { SET_BIT_64(submesh_visibility_flags[submesh_index], view_index); }
    inline void set_submesh_invisible(const uint32_t& submesh_index, const uint32_t& view_index) { CLEAR_BIT_64(submesh_visibility_flags[submesh_index], view_index); }
#endif
};
} // namespace nimble