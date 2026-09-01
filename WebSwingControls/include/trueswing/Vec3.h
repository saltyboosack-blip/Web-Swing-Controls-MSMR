#pragma once

#include <cmath>

namespace trueswing::rebuild {

struct Vec3 final {
    double x{};
    double y{};
    double z{};

    [[nodiscard]] constexpr Vec3 operator+() const { return *this; }
    [[nodiscard]] constexpr Vec3 operator-() const { return {-x, -y, -z}; }
    [[nodiscard]] constexpr Vec3 operator+(const Vec3& other) const {
        return {x + other.x, y + other.y, z + other.z};
    }
    [[nodiscard]] constexpr Vec3 operator-(const Vec3& other) const {
        return {x - other.x, y - other.y, z - other.z};
    }
    [[nodiscard]] constexpr Vec3 operator*(double scalar) const {
        return {x * scalar, y * scalar, z * scalar};
    }
    [[nodiscard]] constexpr Vec3 operator/(double scalar) const {
        return {x / scalar, y / scalar, z / scalar};
    }

    constexpr Vec3& operator+=(const Vec3& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }
    constexpr Vec3& operator-=(const Vec3& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }
    constexpr Vec3& operator*=(double scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    [[nodiscard]] constexpr double Dot(const Vec3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }
    [[nodiscard]] constexpr Vec3 Cross(const Vec3& other) const {
        return {
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x,
        };
    }
    [[nodiscard]] constexpr double LengthSquared() const { return Dot(*this); }
    [[nodiscard]] double Length() const { return std::sqrt(LengthSquared()); }
    [[nodiscard]] bool IsFinite() const {
        return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
    }
};

[[nodiscard]] inline constexpr Vec3 operator*(double scalar, const Vec3& value) {
    return value * scalar;
}

} // namespace trueswing::rebuild
