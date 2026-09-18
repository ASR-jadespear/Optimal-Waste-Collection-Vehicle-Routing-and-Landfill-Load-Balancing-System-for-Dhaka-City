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

        // Font management (Arial)
        static Font fontArial;
        static bool fontLoaded;
        static void initFont();
        static void unloadFont();
        static void drawText(const char *text, float posX, float posY, float fontSize, Color color);
        static int measureText(const char *text, float fontSize);

        // Retained generic widgets
        static void drawPanel(Rectangle bounds, const char *title = nullptr);
        static void drawProgressBar(Rectangle bounds, float percentage, Color barColor, const char *label);
        static bool drawButton(Rectangle bounds, const char *text, bool active = false);
        static void drawMetricCard(Rectangle bounds, const char *title, const char *value,
                                   const char *subtitle, Color accent);
        static void drawTooltip(Vector2 pos, const std::string &title,
                                const std::vector<std::pair<std::string, std::string>> &fields);

        // === NEW Pipeline-Stepper Widgets ===

        // Title bar: 56px high, console name left, live/sim toggle + clock + play/pause right
        // Returns true if play/pause was clicked
        static bool drawTitleBar(Rectangle bounds, bool isPaused, double simTimeSec, float speedMul, bool isLiveMode);

        // Pipeline stepper: 48px, 5 clickable stage buttons
        // Returns clicked stage index (0-4) or -1 if none clicked
        static int drawPipelineStepper(Rectangle bounds, int activeStage);

        // Left icon rail: 64px wide vertical strip with toggle icons
        // layerFlags: [roads, bins, trucks, water] — toggles in/out
        static void drawIconRail(Rectangle bounds, bool layerFlags[4]);

        // Tab button for right panel header
        static bool drawTabButton(Rectangle bounds, const char *text, bool active);

        // Horizontal timeline scrubber. Returns new normalized position [0..1] if dragged, else current.
        static float drawTimelineScrubber(Rectangle bounds, float normalizedPos, bool isPaused);

        // Disruption button with icon character
        static bool drawDisruptionButton(Rectangle bounds, const char *icon, const char *label);

        // Slider widget for parameters tab. Returns new value.
        static float drawSlider(Rectangle bounds, const char *label, float value, float minVal, float maxVal);
    };

} // namespace dhaka
