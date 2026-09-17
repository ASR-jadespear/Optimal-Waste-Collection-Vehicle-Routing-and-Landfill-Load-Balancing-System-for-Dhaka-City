#include "ui/CameraController.hpp"
#include <raymath.h>
#include <algorithm>

namespace dhaka
{

    CameraController::CameraController(int screenWidth, int screenHeight)
    {
        resetView(screenWidth, screenHeight);
    }

    void CameraController::resetView(int screenWidth, int screenHeight)
    {
        camera.target = {530.0f, 480.0f}; // Center of Dhaka network
        camera.offset = {screenWidth * 0.42f, screenHeight * 0.50f};
        camera.rotation = 0.0f;
        camera.zoom = 0.95f;
    }

    void CameraController::update(float dt)
    {
        // Zoom control with mouse wheel
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f)
        {
            Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
            camera.offset = GetMousePosition();
            camera.target = mouseWorldPos;

            const float zoomIncrement = 0.12f;
            camera.zoom += wheel * zoomIncrement;
            camera.zoom = std::clamp(camera.zoom, 0.45f, 2.8f);
        }

        // Drag to pan with Right Mouse Button or Middle Mouse Button
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))
        {
            Vector2 delta = GetMouseDelta();
            delta = Vector2Scale(delta, -1.0f / camera.zoom);
            camera.target = Vector2Add(camera.target, delta);
        }

        // Keyboard Pan with Arrow keys or WASD
        float panSpeed = 400.0f * dt / camera.zoom;
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
            camera.target.y -= panSpeed;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
            camera.target.y += panSpeed;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
            camera.target.x -= panSpeed;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
            camera.target.x += panSpeed;

        // Reset view hotkey 'F'
        if (IsKeyPressed(KEY_F))
        {
            resetView(GetScreenWidth(), GetScreenHeight());
        }
    }

    Vec2 CameraController::screenToWorld(Vector2 screenPos) const
    {
        Vector2 w = GetScreenToWorld2D(screenPos, camera);
        return {w.x, w.y};
    }

    Vector2 CameraController::worldToScreen(Vec2 worldPos) const
    {
        return GetWorldToScreen2D({worldPos.x, worldPos.y}, camera);
    }

} // namespace dhaka
