#include "ui/UIComponents.hpp"
#include <cstdio>
#include <cmath>
#include <algorithm>

namespace dhaka
{

    void UIComponents::drawPanel(Rectangle bounds, const char *title)
    {
        DrawRectangleRounded(bounds, 0.05f, 4, PANEL_BG);
        DrawRectangleRoundedLinesEx(bounds, 0.05f, 4, 1.5f, PANEL_BORDER);

        if (title)
        {
            DrawRectangleRounded({bounds.x, bounds.y, bounds.width, 32.0f}, 0.05f, 4, {30, 36, 50, 255});
            DrawRectangleLinesEx({bounds.x, bounds.y, bounds.width, 32.0f}, 1.0f, PANEL_BORDER);
            DrawText(title, static_cast<int>(bounds.x + 12), static_cast<int>(bounds.y + 8), 15, TEXT_PRIMARY);
        }
    }

    void UIComponents::drawMetricCard(Rectangle bounds, const char *title, const char *value,
                                      const char *subtitle, Color accent)
    {
        DrawRectangleRounded(bounds, 0.08f, 4, PANEL_BG);
        DrawRectangleRoundedLinesEx(bounds, 0.08f, 4, 1.2f, PANEL_BORDER);

        // Accent line on left edge
        DrawRectangle(static_cast<int>(bounds.x), static_cast<int>(bounds.y + 6), 4,
                      static_cast<int>(bounds.height - 12), accent);

        DrawText(title, static_cast<int>(bounds.x + 14), static_cast<int>(bounds.y + 10), 12, TEXT_MUTED);
        DrawText(value, static_cast<int>(bounds.x + 14), static_cast<int>(bounds.y + 26), 20, TEXT_PRIMARY);
        if (subtitle)
        {
            DrawText(subtitle, static_cast<int>(bounds.x + 14), static_cast<int>(bounds.y + 50), 11, TEXT_MUTED);
        }
    }

    void UIComponents::drawProgressBar(Rectangle bounds, float percentage, Color barColor, const char *label)
    {
        percentage = std::clamp(percentage, 0.0f, 100.0f);

        DrawRectangleRounded(bounds, 0.3f, 4, {25, 30, 42, 255});
        DrawRectangleRoundedLinesEx(bounds, 0.3f, 4, 1.0f, PANEL_BORDER);

        float fillWidth = (bounds.width - 4.0f) * (percentage / 100.0f);
        if (fillWidth > 2.0f)
        {
            DrawRectangleRounded({bounds.x + 2.0f, bounds.y + 2.0f, fillWidth, bounds.height - 4.0f},
                                 0.3f, 4, barColor);
        }

        if (label)
        {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%s %.0f%%", label, percentage);
            int textW = MeasureText(buf, 11);
            int textX = static_cast<int>(bounds.x + (bounds.width - textW) * 0.5f);
            int textY = static_cast<int>(bounds.y + (bounds.height - 11) * 0.5f);
            DrawText(buf, textX, textY, 11, WHITE);
        }
    }

    bool UIComponents::drawButton(Rectangle bounds, const char *text, bool active)
    {
        Vector2 mousePos = GetMousePosition();
        bool hovered = CheckCollisionPointRec(mousePos, bounds);
        bool clicked = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

        Color bg = active ? Color{37, 99, 235, 255} : (hovered ? Color{45, 55, 75, 255} : Color{28, 34, 48, 255});
        Color border = active ? Color{96, 165, 250, 255} : (hovered ? COLOR_CYAN : PANEL_BORDER);

        DrawRectangleRounded(bounds, 0.15f, 4, bg);
        DrawRectangleRoundedLinesEx(bounds, 0.15f, 4, 1.2f, border);

        int textW = MeasureText(text, 13);
        int textX = static_cast<int>(bounds.x + (bounds.width - textW) * 0.5f);
        int textY = static_cast<int>(bounds.y + (bounds.height - 13) * 0.5f);
        DrawText(text, textX, textY, 13, hovered ? WHITE : TEXT_PRIMARY);

        return clicked;
    }

    void UIComponents::drawLandfillBalanceGauge(Rectangle bounds,
                                                float aminIntake, float aminCap,
                                                float matIntake, float matCap,
                                                float balanceRatio)
    {
        drawPanel(bounds, "Landfill Load-Balancing (Max-Flow)");

        float pAmin = aminCap > 0.0f ? (aminIntake / aminCap) * 100.0f : 0.0f;
        float pMat = matCap > 0.0f ? (matIntake / matCap) * 100.0f : 0.0f;

        float yOffset = bounds.y + 42.0f;

        // Aminbazar
        char aminText[64];
        std::snprintf(aminText, sizeof(aminText), "Aminbazar (DNCC): %.0f / %.0f kg", aminIntake, aminCap);
        DrawText(aminText, static_cast<int>(bounds.x + 14), static_cast<int>(yOffset), 12, TEXT_PRIMARY);
        Color colAmin = pAmin > 85.0f ? COLOR_RED : (pAmin > 60.0f ? COLOR_GOLD : COLOR_CYAN);
        drawProgressBar({bounds.x + 14, yOffset + 18.0f, bounds.width - 28.0f, 16.0f}, pAmin, colAmin, "");

        yOffset += 45.0f;

        // Matuail
        char matText[64];
        std::snprintf(matText, sizeof(matText), "Matuail (DSCC): %.0f / %.0f kg", matIntake, matCap);
        DrawText(matText, static_cast<int>(bounds.x + 14), static_cast<int>(yOffset), 12, TEXT_PRIMARY);
        Color colMat = pMat > 85.0f ? COLOR_RED : (pMat > 60.0f ? COLOR_GOLD : COLOR_PURPLE);
        drawProgressBar({bounds.x + 14, yOffset + 18.0f, bounds.width - 28.0f, 16.0f}, pMat, colMat, "");

        yOffset += 45.0f;

        // Balance Ratio Meter
        char balText[64];
        float balPct = balanceRatio * 100.0f;
        std::snprintf(balText, sizeof(balText), "Edmonds-Karp Intake Balance: %.1f%%", balPct);
        DrawText(balText, static_cast<int>(bounds.x + 14), static_cast<int>(yOffset), 12,
                 balPct >= 70.0f ? COLOR_GREEN : COLOR_GOLD);
        drawProgressBar({bounds.x + 14, yOffset + 18.0f, bounds.width - 28.0f, 14.0f}, balPct,
                        balPct >= 70.0f ? COLOR_GREEN : COLOR_GOLD, "");
    }

    void UIComponents::drawAlgorithmHUD(Rectangle bounds, const SimulationEngine &engine)
    {
        drawPanel(bounds, "Algorithmic Pipeline Inspector");

        float y = bounds.y + 40.0f;

        // 1. MergeSort / PriorityQueue Urgency Ranking
        DrawText("1. Urgency Management (MergeSort + PriorityQueue)",
                 static_cast<int>(bounds.x + 12), static_cast<int>(y), 12, COLOR_CYAN);
        y += 18.0f;

        int displayed = 0;
        for (const auto *bin : engine.sortedBinsByUrgency)
        {
            if (!bin)
                continue;
            char line[128];
            std::snprintf(line, sizeof(line), "   #%d %s: Urg %.2f (%.0f kg / %.0f%%)",
                          bin->id, bin->name.c_str(), bin->urgencyScore,
                          bin->currentWasteKg, bin->getFillRatio() * 100.0);
            Color binColor = bin->isOverflowing() ? COLOR_RED : (bin->urgencyScore > 0.8 ? COLOR_GOLD : TEXT_MUTED);
            DrawText(line, static_cast<int>(bounds.x + 12), static_cast<int>(y), 11, binColor);
            y += 16.0f;
            if (++displayed >= 3)
                break;
        }

        y += 8.0f;
        DrawLine(static_cast<int>(bounds.x + 12), static_cast<int>(y),
                 static_cast<int>(bounds.x + bounds.width - 12), static_cast<int>(y), PANEL_BORDER);
        y += 8.0f;

        // 2. 0/1 Knapsack DP
        DrawText("2. Target Selection (0/1 Knapsack DP)",
                 static_cast<int>(bounds.x + 12), static_cast<int>(y), 12, COLOR_GOLD);
        y += 18.0f;
        char knapBuf[128];
        std::snprintf(knapBuf, sizeof(knapBuf), "   DP Value: %d | Items Selected: %d / %d",
                      engine.lastKnapsackResult.totalValue,
                      engine.lastKnapsackResult.itemsSelected,
                      engine.lastKnapsackResult.itemsConsidered);
        DrawText(knapBuf, static_cast<int>(bounds.x + 12), static_cast<int>(y), 11, TEXT_PRIMARY);
        y += 16.0f;
        char knapCap[128];
        std::snprintf(knapCap, sizeof(knapCap), "   Payload Packed: %.0f kg (DP Time: %.2f ms)",
                      engine.lastKnapsackResult.totalWeightKg,
                      engine.lastKnapsackResult.executionTimeMs);
        DrawText(knapCap, static_cast<int>(bounds.x + 12), static_cast<int>(y), 11, TEXT_MUTED);

        y += 8.0f;
        DrawLine(static_cast<int>(bounds.x + 12), static_cast<int>(y),
                 static_cast<int>(bounds.x + bounds.width - 12), static_cast<int>(y), PANEL_BORDER);
        y += 8.0f;

        // 3. Routing & Landfill Balancer
        DrawText("3. Routing (A*) & Terminal Balancing (Edmonds-Karp)",
                 static_cast<int>(bounds.x + 12), static_cast<int>(y), 12, COLOR_GREEN);
        y += 18.0f;
        char flowBuf[128];
        std::snprintf(flowBuf, sizeof(flowBuf), "   Augmenting Paths: %d | Balanced Quotas: %s",
                      engine.lastMaxFlowResult.augmentingPathsCount,
                      engine.lastMaxFlowResult.isBalanced ? "Optimal 50/50" : "Compensating");
        DrawText(flowBuf, static_cast<int>(bounds.x + 12), static_cast<int>(y), 11, TEXT_PRIMARY);

        y += 24.0f;
        // Live Algorithm ticker / log
        DrawRectangle(static_cast<int>(bounds.x + 8), static_cast<int>(y),
                      static_cast<int>(bounds.width - 16), 24, {20, 24, 34, 255});
        DrawText(engine.lastAlgorithmStatusMessage.c_str(),
                 static_cast<int>(bounds.x + 14), static_cast<int>(y + 6), 11, COLOR_CYAN);
    }

    void UIComponents::drawTooltip(Vector2 pos, const std::string &title,
                                   const std::vector<std::pair<std::string, std::string>> &fields)
    {
        float width = 230.0f;
        float height = 30.0f + fields.size() * 18.0f;

        // Keep on screen
        if (pos.x + width > GetScreenWidth() - 10)
            pos.x = GetScreenWidth() - width - 10;
        if (pos.y + height > GetScreenHeight() - 10)
            pos.y = GetScreenHeight() - height - 10;

        DrawRectangleRounded({pos.x, pos.y, width, height}, 0.1f, 4, {18, 22, 32, 245});
        DrawRectangleRoundedLinesEx({pos.x, pos.y, width, height}, 0.1f, 4, 1.2f, COLOR_CYAN);

        DrawText(title.c_str(), static_cast<int>(pos.x + 10), static_cast<int>(pos.y + 8), 13, COLOR_CYAN);

        float y = pos.y + 28.0f;
        for (const auto &[k, v] : fields)
        {
            DrawText(k.c_str(), static_cast<int>(pos.x + 10), static_cast<int>(y), 11, TEXT_MUTED);
            int valW = MeasureText(v.c_str(), 11);
            DrawText(v.c_str(), static_cast<int>(pos.x + width - valW - 10), static_cast<int>(y), 11, WHITE);
            y += 18.0f;
        }
    }

} // namespace dhaka
