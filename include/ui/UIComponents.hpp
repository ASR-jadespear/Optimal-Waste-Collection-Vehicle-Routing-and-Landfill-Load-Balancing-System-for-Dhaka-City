#pragma once

#include "core/Types.hpp"
#include "simulation/SimulationEngine.hpp"
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
        static constexpr Color COLOR_CYAN = {6, 182, 212, 255};    // DNCC / General accent
        static constexpr Color COLOR_GOLD = {245, 158, 11, 255};   // DSCC / Warnings
        static constexpr Color COLOR_GREEN = {16, 185, 129, 255};  // Optimal / Low traffic
        static constexpr Color COLOR_RED = {239, 68, 68, 255};     // Overflow / Gridlock
        static constexpr Color COLOR_PURPLE = {168, 85, 247, 255}; // Landfill / Algorithm

        static void drawPanel(Rectangle bounds, const char *title = nullptr);

        static void drawMetricCard(Rectangle bounds, const char *title, const char *value,
                                   const char *subtitle, Color accent);

        static void drawProgressBar(Rectangle bounds, float percentage, Color barColor, const char *label);

        static bool drawButton(Rectangle bounds, const char *text, bool active = false);

        static void drawLandfillBalanceGauge(Rectangle bounds,
                                             float aminIntake, float aminCap,
                                             float matIntake, float matCap,
                                             float balanceRatio);

        static void drawAlgorithmHUD(Rectangle bounds, const SimulationEngine &engine);

        static void drawTooltip(Vector2 pos, const std::string &title,
                                const std::vector<std::pair<std::string, std::string>> &fields);
    };

} // namespace dhaka
