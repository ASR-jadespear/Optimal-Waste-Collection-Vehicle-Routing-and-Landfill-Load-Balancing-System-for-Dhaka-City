#pragma once

#include "core/Types.hpp"
#include <raylib.h>

namespace dhaka
{

    class CameraController
    {
    public:
        Camera2D camera;
        bool isDragging = false;
        Vector2 dragStartPos = {0.0f, 0.0f};

        CameraController(int screenWidth = 1440, int screenHeight = 900);

        void update(float dt);
        void resetView(int screenWidth = 1440, int screenHeight = 900);
        Vec2 screenToWorld(Vector2 screenPos) const;
        Vector2 worldToScreen(Vec2 worldPos) const;
    };

} // namespace dhaka
