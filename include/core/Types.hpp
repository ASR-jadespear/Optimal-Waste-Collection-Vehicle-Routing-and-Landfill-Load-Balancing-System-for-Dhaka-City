#pragma once

#include <string>
#include <cmath>
#include <algorithm>

namespace dhaka
{

    struct Vec2
    {
        float x = 0.0f;
        float y = 0.0f;

        Vec2() = default;
        constexpr Vec2(float x_, float y_) : x(x_), y(y_) {}

        Vec2 operator+(const Vec2 &o) const { return {x + o.x, y + o.y}; }
        Vec2 operator-(const Vec2 &o) const { return {x - o.x, y - o.y}; }
        Vec2 operator*(float scalar) const { return {x * scalar, y * scalar}; }
        Vec2 operator/(float scalar) const { return {x / scalar, y / scalar}; }

        float lengthSquared() const { return x * x + y * y; }
        float length() const { return std::sqrt(lengthSquared()); }

        float distanceTo(const Vec2 &o) const
        {
            return (*this - o).length();
        }

        Vec2 normalized() const
        {
            float l = length();
            if (l > 1e-6f)
                return {x / l, y / l};
            return {0.0f, 0.0f};
        }

        static Vec2 lerp(const Vec2 &a, const Vec2 &b, float t)
        {
            t = std::clamp(t, 0.0f, 1.0f);
            return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
        }
    };

    enum class Corporation
    {
        DNCC,    // Dhaka North City Corporation
        DSCC,    // Dhaka South City Corporation
        COMBINED // Both / Inter-Corporation
    };

    inline std::string corporationToString(Corporation corp)
    {
        switch (corp)
        {
        case Corporation::DNCC:
            return "DNCC";
        case Corporation::DSCC:
            return "DSCC";
        default:
            return "Dhaka City";
        }
    }

    enum class VehicleState
    {
        IDLE_AT_DEPOT,
        EN_ROUTE_TO_BIN,
        COLLECTING_WASTE,
        EN_ROUTE_TO_LANDFILL,
        UNLOADING_AT_LANDFILL,
        RETURNING_TO_DEPOT
    };

    inline std::string vehicleStateToString(VehicleState state)
    {
        switch (state)
        {
        case VehicleState::IDLE_AT_DEPOT:
            return "Idle at Depot";
        case VehicleState::EN_ROUTE_TO_BIN:
            return "En Route to Bin";
        case VehicleState::COLLECTING_WASTE:
            return "Collecting Waste";
        case VehicleState::EN_ROUTE_TO_LANDFILL:
            return "Heading to Landfill";
        case VehicleState::UNLOADING_AT_LANDFILL:
            return "Unloading";
        case VehicleState::RETURNING_TO_DEPOT:
            return "Returning to Depot";
        default:
            return "Unknown";
        }
    }

    enum class NodeType
    {
        INTERSECTION,
        DEPOT,
        COLLECTION_POINT,
        LANDFILL
    };

} // namespace dhaka
