#pragma once

#include "core/Types.hpp"
#include <raylib.h>
#include <string>
#include <vector>

namespace dhaka
{

    class UIComponents
    {
    public:
        // Color Palette
        static constexpr Color BG_DARK = {15, 18, 26, 255};
        static constexpr Color PANEL_BG = {23, 27, 38, 240};
        static constexpr Color PANEL_BORDER = {38, 45, 61, 255};
        static constexpr Color TEXT_PRIMARY = {248, 250, 252, 255};
        static constexpr Color TEXT_MUTED = {148, 163, 184, 255};
        static constexpr Color COLOR_CYAN = {6, 182, 212, 255};
        static constexpr Color COLOR_GOLD = {245, 158, 11, 255};
        static constexpr Color COLOR_GREEN = {16, 185, 129, 255};
        static constexpr Color COLOR_RED = {239, 68, 68, 255};
        static constexpr Color COLOR_PURPLE = {168, 85, 247, 255};
        static constexpr Color COLOR_GRAY = {100, 116, 139, 255};

        // Font management (Arial)
        static Font fontArial;
        static bool fontLoaded;
        static void initFont();
        static void unloadFont();
        static void drawText(const char *text, float posX, float posY, float fontSize, Color color);
        static int measureText(const char *text, float fontSize);

        // Generic & HUD widgets
        static void drawPanel(Rectangle bounds, const char *title = nullptr);
        static void drawProgressBar(Rectangle bounds, float percentage, Color barColor, const char *label);
        static bool drawButton(Rectangle bounds, const char *text, bool active = false);
        static void drawMetricCard(Rectangle bounds, const char *title, const char *value,
                                   const char *subtitle, Color accent);
        static void drawTooltip(Vector2 pos, const std::string &title,
                                const std::vector<std::pair<std::string, std::string>> &fields);

        // Pipeline-Stepper Widgets
        static bool drawTitleBar(Rectangle bounds, bool isPaused, double simTimeSec, float speedMul, bool &isLiveMode);
        static int drawPipelineStepper(Rectangle bounds, int activeStage);
        static int drawIconRail(Rectangle bounds, bool layerFlags[4], int alertCount, int activePopover);
        static bool drawTabButton(Rectangle bounds, const char *text, bool active);
        static float drawTimelineScrubber(Rectangle bounds, float normalizedPos, bool isPaused);
        static bool drawDisruptionButton(Rectangle bounds, const char *icon, const char *label, bool active = false);
        static float drawSlider(Rectangle bounds, const char *label, float value, float minVal, float maxVal);

        // Step-Through Debugger Controls
        static int drawStepControls(Rectangle bounds, bool isPlaying, int currentStep, int totalSteps);

        // Visual Encodings
        static void drawDashedLine(Vector2 start, Vector2 end, float thick, float dashLen, Color color);
        static void drawRadialGauge(Vector2 center, float innerRadius, float outerRadius, float percentage, Color barColor, Color bgColor);
    };

} // namespace dhaka
