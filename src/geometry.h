#pragma once

#include <glm.hpp>

namespace nimble
{
#define CMP(x, y) \
    (fabsf(x - y) <= FLT_EPSILON * fmaxf(1.0f, fmaxf(fabsf(x), fabsf(y))))

enum FrustumPlanes
{
    FRUSTUM_PLANE_NEAR   = 0,
    FRUSTUM_PLANE_FAR    = 1,
    FRUSTUM_PLANE_LEFT   = 2,
    FRUSTUM_PLANE_RIGHT  = 3,
    FRUSTUM_PLANE_TOP    = 4,
    FRUSTUM_PLANE_BOTTOM = 5
};

struct Plane
{
    glm::vec3 normal;
    float     distance;
};

struct Frustum
{
    Plane planes[6];
};

struct Sphere
{
    glm::vec3 position;
    float     radius;
};

struct AABB
{
    glm::vec3 min;
    glm::vec3 max;
};

struct OBB
{
    glm::vec3 position;
    glm::vec3 min;
    glm::vec3 max;
    glm::mat3 orientation;
};

struct Ray
{
    glm::vec3 origin;
    glm::vec3 direction;

    Ray(glm::vec3 o, glm::vec3 d) :
        origin(o), direction(d) {}
};

inline void frustum_from_matrix(Frustum& frustum, const glm::mat4& view_proj)
{
    frustum.planes[FRUSTUM_PLANE_RIGHT].normal   = glm::vec3(view_proj[0][3] - view_proj[0][0],
                                                           view_proj[1][3] - view_proj[1][0],
                                                           view_proj[2][3] - view_proj[2][0]);
    frustum.planes[FRUSTUM_PLANE_RIGHT].distance = view_proj[3][3] - view_proj[3][0];

    frustum.planes[FRUSTUM_PLANE_LEFT].normal   = glm::vec3(view_proj[0][3] + view_proj[0][0],
                                                          view_proj[1][3] + view_proj[1][0],
                                                          view_proj[2][3] + view_proj[2][0]);
    frustum.planes[FRUSTUM_PLANE_LEFT].distance = view_proj[3][3] + view_proj[3][0];

    frustum.planes[FRUSTUM_PLANE_BOTTOM].normal   = glm::vec3(view_proj[0][3] + view_proj[0][1],
                                                            view_proj[1][3] + view_proj[1][1],
                                                            view_proj[2][3] + view_proj[2][1]);
    frustum.planes[FRUSTUM_PLANE_BOTTOM].distance = view_proj[3][3] + view_proj[3][1];

    frustum.planes[FRUSTUM_PLANE_TOP].normal   = glm::vec3(view_proj[0][3] - view_proj[0][1],
                                                         view_proj[1][3] - view_proj[1][1],
                                                         view_proj[2][3] - view_proj[2][1]);
    frustum.planes[FRUSTUM_PLANE_TOP].distance = view_proj[3][3] - view_proj[3][1];

    frustum.planes[FRUSTUM_PLANE_FAR].normal   = glm::vec3(view_proj[0][3] - view_proj[0][2],
                                                         view_proj[1][3] - view_proj[1][2],
                                                         view_proj[2][3] - view_proj[2][2]);
    frustum.planes[FRUSTUM_PLANE_FAR].distance = view_proj[3][3] - view_proj[3][2];

    frustum.planes[FRUSTUM_PLANE_NEAR].normal   = glm::vec3(view_proj[0][3] + view_proj[0][2],
                                                          view_proj[1][3] + view_proj[1][2],
                                                          view_proj[2][3] + view_proj[2][2]);
    frustum.planes[FRUSTUM_PLANE_NEAR].distance = view_proj[3][3] + view_proj[3][2];

    // Normalize them
    for (int i = 0; i < 6; i++)
    {
        float invl = sqrt(frustum.planes[i].normal.x * frustum.planes[i].normal.x + frustum.planes[i].normal.y * frustum.planes[i].normal.y + frustum.planes[i].normal.z * frustum.planes[i].normal.z);
        frustum.planes[i].normal /= invl;
        frustum.planes[i].distance /= invl;
    }
}

inline void extract_frustum_corners(const glm::mat4& inv_view_proj, glm::vec3* corners)
{
    const glm::vec4 kFrustumCorners[] = {
        glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f),
        glm::vec4(-1.0f, 1.0f, 1.0f, 1.0f),
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        glm::vec4(1.0f, -1.0f, 1.0f, 1.0f),
        glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f),
        glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f),
        glm::vec4(1.0f, 1.0f, -1.0f, 1.0f),
        glm::vec4(1.0f, -1.0f, -1.0f, 1.0f)
    };

    for (int i = 0; i < 8; i++)
    {
        glm::vec4 v = inv_view_proj * kFrustumCorners[i];
        v           = v / v.w;
        corners[i]  = glm::vec3(v.x, v.y, v.z);
    }
}

// https://github.com/gszauer/GamePhysicsCookbook/blob/master/Code/Geometry3D.cpp

inline float classify(const Plane& plane, const AABB& aabb)
{
    glm::vec3 center  = (aabb.max + aabb.min) / 2.0f;
    glm::vec3 extents = aabb.max - aabb.min;

    float r = fabsf(extents.x * plane.normal.x) + fabsf(extents.y * plane.normal.y) + fabsf(extents.z * plane.normal.z);

    float d = glm::dot(plane.normal, center) + plane.distance;

    if (fabsf(d) < r)
        return 0.0f;
    else if (d < 0.0f)
        return d + r;
    else
        return d - r;
}

inline float classify(const OBB& obb, const Plane& plane)
{
    glm::vec3 center  = (obb.max + obb.min) / 2.0f;
    glm::vec3 extents = obb.max - obb.min;

    glm::vec3 normal = obb.orientation * plane.normal;

    // maximum extent in direction of plane normal
    float r = fabsf(extents.x * normal.x)
        + fabsf(extents.y * normal.y)
        + fabsf(extents.z * normal.z);

    // signed distance between box center and plane
    //float d = plane.Test(mCenter);
    float d = glm::dot(plane.normal, obb.position) + plane.distance;

    // return signed distance
    if (fabsf(d) < r)
        return 0.0f;
    else if (d < 0.0f)
        return d + r;

    return d - r;
}

inline bool intersects(const Frustum& f, const Sphere& s)
{
    for (int i = 0; i < 6; ++i)
    {
        glm::vec3 normal = f.planes[i].normal;
        float     dist   = f.planes[i].distance;

        float side = glm::dot(s.position, normal) + dist;

        if (side < -s.radius)
            return false;
    }

    return true;
}

inline bool intersects(const Frustum& frustum, const AABB& aabb)
{
    for (int i = 0; i < 6; i++)
    {
        if (classify(frustum.planes[i], aabb) < 0.0f)
            return false;
    }

    return true;
}

inline bool intersects(const Frustum& f, const OBB& obb)
{
    for (int i = 0; i < 6; i++)
    {
        float side = classify(obb, f.planes[i]);

        if (side < 0)
            return false;
    }
    return true;
}

inline glm::vec3 unproject(const glm::vec3& viewportPoint, const glm::vec2& viewportOrigin, const glm::vec2& viewportSize, const glm::mat4& view, const glm::mat4& projection)
{
    // Step 1, Normalize the input vector to the view port
    glm::vec4 normalized = {
        (viewportPoint.x - viewportOrigin.x) / viewportSize.x,
        (viewportPoint.y - viewportOrigin.y) / viewportSize.y,
        viewportPoint.z,
        1.0f
    };

    // Step 2, Translate into NDC space
    glm::vec4 ndcSpace = {
        normalized[0], normalized[1], normalized[2], normalized[3]
    };
    // X Range: -1 to 1
    ndcSpace[0] = ndcSpace[0] * 2.0f - 1.0f;
    // Y Range: -1 to 1, our Y axis is flipped!
    ndcSpace[1] = 1.0f - ndcSpace[1] * 2.0f;
    // Z Range: 0 to 1
    if (ndcSpace[2] < 0.0f)
        ndcSpace[2] = 0.0f;

    if (ndcSpace[2] > 1.0f)
        ndcSpace[2] = 1.0f;

    // Step 3, NDC to Eye Space
    glm::mat4 invProjection = glm::inverse(projection);
    glm::vec4 eyeSpace      = invProjection * ndcSpace;

    // Step 4, Eye Space to World Space
    glm::mat4 invView    = glm::inverse(view);
    glm::vec4 worldSpace = invView * eyeSpace;

    // Step 5, Undo perspective divide!
    if (!CMP(worldSpace[3], 0.0f))
    {
        worldSpace[0] /= worldSpace[3];
        worldSpace[1] /= worldSpace[3];
        worldSpace[2] /= worldSpace[3];
    }

    // Return the resulting world space point
    return glm::vec3(worldSpace[0], worldSpace[1], worldSpace[2]);
}

inline Ray picking_ray(const glm::vec2& viewportPoint, const glm::vec2& viewportOrigin, const glm::vec2& viewportSize, const glm::mat4& view, const glm::mat4& projection)
{
    glm::vec3 nearPoint(viewportPoint.x, viewportPoint.y, 0.0f);
    glm::vec3 farPoint(viewportPoint.x, viewportPoint.y, 1.0f);

    glm::vec3 pNear = unproject(nearPoint, viewportOrigin, viewportSize, view, projection);
    glm::vec3 pFar  = unproject(farPoint, viewportOrigin, viewportSize, view, projection);

    glm::vec3 normal = glm::normalize(pFar - pNear);
    glm::vec3 origin = pNear;

    return Ray(origin, normal);
}
} // namespace nimble
