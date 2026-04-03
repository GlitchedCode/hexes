#pragma once

#include <glaze/glaze.hpp>

#include <array>
#include <cmath>

// A 2D transform stored as position / rotation / scale (TRS).
//
// The equivalent affine matrix (column-major, homogeneous 3×3) is:
//
//   | sx·cos(r)  -sy·sin(r)  tx |
//   | sx·sin(r)   sy·cos(r)  ty |
//   |     0           0       1 |
//
// Serialization example (glz::meta provided below):
//
//   hexes::Transform2D t{.position = {1.f, 2.f}, .rotation = 0.5f, .scale = {1.f, 1.f}};
//   auto json = hexes::to_json(t);   // works out of the box

namespace hexes {

struct Vec2 {
    float x = 0.f;
    float y = 0.f;

    Vec2 operator+(Vec2 o) const { return {x + o.x, y + o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
};

struct Transform2D {
    Vec2  position = {};
    float rotation = 0.f; // radians
    Vec2  scale    = {1.f, 1.f};

    // Returns the 3×3 affine matrix in row-major order (9 elements).
    std::array<float, 9> to_matrix() const {
        const float c  = std::cos(rotation);
        const float s  = std::sin(rotation);
        const float sx = scale.x, sy = scale.y;
        const float tx = position.x, ty = position.y;
        return {
            sx * c, -sy * s, tx,
            sx * s,  sy * c, ty,
            0.f,     0.f,    1.f,
        };
    }

    // Apply this transform to a 2D point.
    Vec2 apply(Vec2 p) const {
        const float c  = std::cos(rotation);
        const float s  = std::sin(rotation);
        return {
            scale.x * (c * p.x - s * p.y) + position.x,
            scale.y * (s * p.x + c * p.y) + position.y,
        };
    }

    // Compose: returns a transform equivalent to applying `other` first, then `this`.
    Transform2D compose(const Transform2D& other) const {
        Transform2D result;
        result.rotation   = rotation + other.rotation;
        result.scale      = {scale.x * other.scale.x, scale.y * other.scale.y};
        result.position   = apply({other.position.x, other.position.y});
        return result;
    }

    static Transform2D identity() { return {}; }
};

} // namespace hexes

// ── glz::meta specialisations (enables JSON / YAML / binary serialization) ────

template <> struct glz::meta<hexes::Vec2> {
    using T = hexes::Vec2;
    static constexpr auto value = glz::object("x", &T::x, "y", &T::y);
};

template <> struct glz::meta<hexes::Transform2D> {
    using T = hexes::Transform2D;
    static constexpr auto value =
        glz::object("position", &T::position, "rotation", &T::rotation, "scale", &T::scale);
};
