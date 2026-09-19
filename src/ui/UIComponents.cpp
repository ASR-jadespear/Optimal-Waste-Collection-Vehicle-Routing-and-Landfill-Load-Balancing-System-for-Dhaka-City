#include "ui/UIComponents.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <vector>

namespace dhaka
{

    Font UIComponents::fontArial = {};
    bool UIComponents::fontLoaded = false;

    void UIComponents::initFont()
    {
        if (fontLoaded)
            return;

        const std::vector<std::string> fontCandidates = {
            "assets/fonts/arial.ttf",
            "assets/fonts/arial.otf",
            "C:/Windows/Fonts/arial.ttf",
            "C:/Windows/Fonts/Arial.ttf",
            "/usr/share/fonts/urw-base35/NimbusSans-Regular.otf",
            "/usr/share/fonts/open-sans/OpenSans-Regular.ttf",
            "/usr/share/fonts/google-noto/NotoSans-Regular.ttf"};

        for (const auto &p : fontCandidates)
        {
            if (FileExists(p.c_str()))
            {
                fontArial = LoadFontEx(p.c_str(), 48, nullptr, 0);
                if (fontArial.texture.id > 0)
                {
                    SetTextureFilter(fontArial.texture, TEXTURE_FILTER_BILINEAR);
                    fontLoaded = true;
                    std::cout << "[SYSTEM] Loaded Arial font successfully from: " << p << "\n";
                    return;
                }
            }
        }

        fontArial = GetFontDefault();
        fontLoaded = false;
        std::cout << "[SYSTEM] Arial font file not found, using default fallback font.\n";
    }

    void UIComponents::unloadFont()
    {
        if (fontLoaded)
        {
            UnloadFont(fontArial);
            fontLoaded = false;
        }
    }

    void UIComponents::drawText(const char *text, float posX, float posY, float fontSize, Color color)
    {
        if (!text || text[0] == '\0')
            return;

        if (fontLoaded)
        {
            DrawTextEx(fontArial, text, {posX, posY}, fontSize, 1.0f, color);
        }
        else
        {
            DrawText(text, static_cast<int>(posX), static_cast<int>(posY), static_cast<int>(fontSize), color);
        }
    }

    int UIComponents::measureText(const char *text, float fontSize)
    {
        if (!text || text[0] == '\0')
            return 0;

        if (fontLoaded)
        {
            Vector2 size = MeasureTextEx(fontArial, text, fontSize, 1.0f);
            return static_cast<int>(std::ceil(size.x));
        }
        else
        {
            return MeasureText(text, static_cast<int>(fontSize));
        }
    }

    void UIComponents::drawDashedLine(Vector2 start, Vector2 end, float thick, float dashLen, Color color)
    {
        float dx = end.x - start.x;
        float dy = end.y - start.y;
        float totalLen = std::sqrt(dx * dx + dy * dy);
        if (totalLen <= 0.001f)
            return;

        float ux = dx / totalLen;
        float uy = dy / totalLen;

        float current = 0.0f;
        bool draw = true;

        while (current < totalLen)
        {
            float next = std::min(totalLen, current + dashLen);
            if (draw)
            {
                Vector2 p1 = {start.x + ux * current, start.y + uy * current};
                Vector2 p2 = {start.x + ux * next, start.y + uy * next};
                DrawLineEx(p1, p2, thick, color);
            }
            current = next;
            draw = !draw;
        }
    }

    void UIComponents::drawRadialGauge(Vector2 center, float innerRadius, float outerRadius, float percentage, Color barColor, Color bgColor)
    {
        percentage = std::clamp(percentage, 0.0f, 100.0f);
        // Draw background ring
        DrawRing(center, innerRadius, outerRadius, 0.0f, 360.0f, 32, bgColor);
        // Draw progress arc
        float angle = (percentage / 100.0f) * 360.0f;
        if (angle > 1.0f)
        {
            DrawRing(center, innerRadius, outerRadius, 0.0f, angle, 32, barColor);
        }
    }

    void UIComponents::drawPanel(Rectangle bounds, const char *title)
    {
        DrawRectangleRounded(bounds, 0.05f, 4, PANEL_BG);
        DrawRectangleRoundedLinesEx(bounds, 0.05f, 4, 1.5f, PANEL_BORDER);
        if (title)
        {
            DrawRectangleRounded({bounds.x, bounds.y, bounds.width, 32.0f}, 0.05f, 4, {30, 36, 50, 255});
            DrawRectangleLinesEx({bounds.x, bounds.y, bounds.width, 32.0f}, 1.0f, PANEL_BORDER);
            drawText(title, bounds.x + 12.0f, bounds.y + 8.0f, 15.0f, TEXT_PRIMARY);
        }
    }

    void UIComponents::drawMetricCard(Rectangle bounds, const char *title, const char *value,
                                      const char *subtitle, Color accent)
    {
        DrawRectangleRounded(bounds, 0.08f, 4, PANEL_BG);
        DrawRectangleRoundedLinesEx(bounds, 0.08f, 4, 1.2f, PANEL_BORDER);
        DrawRectangle(static_cast<int>(bounds.x), static_cast<int>(bounds.y + 6), 4,
                      static_cast<int>(bounds.height - 12), accent);
        drawText(title, bounds.x + 14.0f, bounds.y + 10.0f, 12.0f, TEXT_MUTED);
        drawText(value, bounds.x + 14.0f, bounds.y + 26.0f, 20.0f, TEXT_PRIMARY);
        if (subtitle)
        {
            drawText(subtitle, bounds.x + 14.0f, bounds.y + 50.0f, 11.0f, TEXT_MUTED);
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
            int textW = measureText(buf, 11.0f);
            float textX = bounds.x + (bounds.width - textW) * 0.5f;
            float textY = bounds.y + (bounds.height - 11.0f) * 0.5f;
            drawText(buf, textX, textY, 11.0f, WHITE);
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
        int textW = measureText(text, 13.0f);
        float textX = bounds.x + (bounds.width - textW) * 0.5f;
        float textY = bounds.y + (bounds.height - 13.0f) * 0.5f;
        drawText(text, textX, textY, 13.0f, hovered ? WHITE : TEXT_PRIMARY);
        return clicked;
    }

    void UIComponents::drawTooltip(Vector2 pos, const std::string &title,
                                   const std::vector<std::pair<std::string, std::string>> &fields)
    {
        float width = 240.0f;
        float height = 32.0f + fields.size() * 18.0f;
        if (pos.x + width > GetScreenWidth() - 10)
            pos.x = GetScreenWidth() - width - 10;
        if (pos.y + height > GetScreenHeight() - 10)
            pos.y = GetScreenHeight() - height - 10;

        DrawRectangleRounded({pos.x, pos.y, width, height}, 0.1f, 4, {18, 22, 32, 245});
        DrawRectangleRoundedLinesEx({pos.x, pos.y, width, height}, 0.1f, 4, 1.2f, COLOR_CYAN);
        drawText(title.c_str(), pos.x + 10.0f, pos.y + 8.0f, 13.0f, COLOR_CYAN);
        float y = pos.y + 28.0f;
        for (const auto &[k, v] : fields)
        {
            drawText(k.c_str(), pos.x + 10.0f, y, 11.0f, TEXT_MUTED);
            int valW = measureText(v.c_str(), 11.0f);
            drawText(v.c_str(), pos.x + width - valW - 10.0f, y, 11.0f, WHITE);
            y += 18.0f;
        }
    }

    bool UIComponents::drawTitleBar(Rectangle bounds, bool isPaused, double simTimeSec, float speedMul, bool &isLiveMode)
    {
        DrawRectangleRec(bounds, PANEL_BG);
        DrawLineEx({bounds.x, bounds.y + bounds.height - 1.0f}, {bounds.x + bounds.width, bounds.y + bounds.height - 1.0f}, 1.0f, PANEL_BORDER);

        // Left Console Icon & Title
        DrawCircle(static_cast<int>(bounds.x + 24), static_cast<int>(bounds.y + bounds.height / 2.0f), 8.0f, COLOR_CYAN);
        drawText("Dhaka SWM command console", bounds.x + 40.0f, bounds.y + bounds.height / 2.0f - 8.0f, 16.0f, TEXT_PRIMARY);

        // Right side: Interactive Live/Simulated toggle pill
        float toggleW = 124.0f;
        float toggleH = 26.0f;
        float toggleX = bounds.x + bounds.width - 340.0f;
        float toggleY = bounds.y + (bounds.height - toggleH) / 2.0f;
        Rectangle toggleBounds = {toggleX, toggleY, toggleW, toggleH};

        bool toggleHovered = CheckCollisionPointRec(GetMousePosition(), toggleBounds);
        if (toggleHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            isLiveMode = !isLiveMode;
        }

        DrawRectangleRounded(toggleBounds, 0.4f, 4, {22, 28, 40, 255});
        DrawRectangleRoundedLinesEx(toggleBounds, 0.4f, 4, 1.2f, toggleHovered ? COLOR_CYAN : PANEL_BORDER);

        if (isLiveMode)
        {
            DrawRectangleRounded({toggleX + 2.0f, toggleY + 2.0f, toggleW * 0.48f, toggleH - 4.0f}, 0.35f, 4, COLOR_GREEN);
            drawText("LIVE", toggleX + 12.0f, toggleY + 6.0f, 11.0f, WHITE);
            drawText("Sim", toggleX + toggleW * 0.55f, toggleY + 6.0f, 11.0f, TEXT_MUTED);
        }
        else
        {
            DrawRectangleRounded({toggleX + toggleW * 0.50f, toggleY + 2.0f, toggleW * 0.48f, toggleH - 4.0f}, 0.35f, 4, COLOR_CYAN);
            drawText("Live", toggleX + 10.0f, toggleY + 6.0f, 11.0f, TEXT_MUTED);
            drawText("SIM", toggleX + toggleW * 0.56f, toggleY + 6.0f, 11.0f, WHITE);
        }

        // Current sim clock: e.g. "14:32, Tue"
        int currentDay = static_cast<int>(simTimeSec / 86400) % 7;
        int hour = static_cast<int>(std::fmod(simTimeSec, 86400) / 3600);
        int minute = static_cast<int>(std::fmod(simTimeSec, 3600) / 60);
        const char *days[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};

        char timeStr[128];
        std::snprintf(timeStr, sizeof(timeStr), "%02d:%02d, %s", hour, minute, days[currentDay]);
        drawText(timeStr, toggleX + toggleW + 18.0f, bounds.y + bounds.height / 2.0f - 7.0f, 14.0f, TEXT_PRIMARY);

        // Play / Pause global button
        Rectangle btnBounds = {bounds.x + bounds.width - 50.0f, bounds.y + (bounds.height - 24.0f) / 2.0f, 26.0f, 24.0f};
        bool hovered = CheckCollisionPointRec(GetMousePosition(), btnBounds);
        bool clicked = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

        Color iconColor = hovered ? TEXT_PRIMARY : TEXT_MUTED;
        if (isPaused)
        {
            Vector2 p1 = {btnBounds.x + 6.0f, btnBounds.y + 4.0f};
            Vector2 p2 = {btnBounds.x + 6.0f, btnBounds.y + 20.0f};
            Vector2 p3 = {btnBounds.x + 20.0f, btnBounds.y + 12.0f};
            DrawTriangle(p1, p2, p3, iconColor);
        }
        else
        {
            DrawRectangle(static_cast<int>(btnBounds.x + 6), static_cast<int>(btnBounds.y + 4), 4, 16, iconColor);
            DrawRectangle(static_cast<int>(btnBounds.x + 14), static_cast<int>(btnBounds.y + 4), 4, 16, iconColor);
        }

        return clicked;
    }

    int UIComponents::drawPipelineStepper(Rectangle bounds, int activeStage)
    {
        DrawRectangleRec(bounds, BG_DARK);
        DrawLineEx({bounds.x, bounds.y + bounds.height - 1.0f}, {bounds.x + bounds.width, bounds.y + bounds.height - 1.0f}, 1.0f, PANEL_BORDER);

        int clickedStage = -1;
        const char *labels[] = {
            "Road network\nBase G(V,E,t)",
            "Routing\nDijkstra / A*",
            "Sequencing\nGreedy",
            "Load select\nKnapsack",
            "Landfill balance\nMax-flow"};
        int numStages = 5;
        float gap = 8.0f;
        float btnWidth = (bounds.width - (numStages - 1) * gap - 24.0f) / numStages;

        for (int i = 0; i < numStages; i++)
        {
            Rectangle btnRec = {bounds.x + 12.0f + i * (btnWidth + gap), bounds.y + 4.0f, btnWidth, bounds.height - 8.0f};
            bool hovered = CheckCollisionPointRec(GetMousePosition(), btnRec);
            if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                clickedStage = i;
            }

            Color bg = (i == activeStage) ? Color{37, 99, 235, 255} : (hovered ? Color{45, 55, 75, 255} : PANEL_BG);
            Color textCol = (i == activeStage) ? WHITE : TEXT_MUTED;

            DrawRectangleRounded(btnRec, 0.12f, 4, bg);
            DrawRectangleRoundedLinesEx(btnRec, 0.12f, 4, 1.2f, (i == activeStage) ? Color{96, 165, 250, 255} : PANEL_BORDER);

            std::string label = labels[i];
            size_t nlPos = label.find('\n');
            if (nlPos != std::string::npos)
            {
                std::string line1 = label.substr(0, nlPos);
                std::string line2 = label.substr(nlPos + 1);
                int w1 = measureText(line1.c_str(), 12.0f);
                int w2 = measureText(line2.c_str(), 10.0f);
                drawText(line1.c_str(), btnRec.x + (btnWidth - w1) / 2.0f, btnRec.y + 6.0f, 12.0f, textCol);
                drawText(line2.c_str(), btnRec.x + (btnWidth - w2) / 2.0f, btnRec.y + 21.0f, 10.0f, (i == activeStage) ? Color{219, 234, 254, 255} : COLOR_GRAY);
            }
            else
            {
                int w = measureText(label.c_str(), 12.0f);
                drawText(label.c_str(), btnRec.x + (btnWidth - w) / 2.0f, btnRec.y + (btnRec.height - 12.0f) / 2.0f, 12.0f, textCol);
            }
        }
        return clickedStage;
    }

    int UIComponents::drawIconRail(Rectangle bounds, bool layerFlags[4], int alertCount, int activePopover)
    {
        DrawRectangleRec(bounds, {18, 22, 32, 255});
        DrawLineEx({bounds.x + bounds.width - 1.0f, bounds.y}, {bounds.x + bounds.width - 1.0f, bounds.y + bounds.height}, 1.0f, PANEL_BORDER);

        int clickedAction = 0;
        float btnSize = 44.0f;
        float gap = 12.0f;
        float startY = bounds.y + 12.0f;

        // 1. Layers Toggle Icon
        Rectangle recLayers = {bounds.x + (bounds.width - btnSize) / 2.0f, startY, btnSize, btnSize};
        bool hLayers = CheckCollisionPointRec(GetMousePosition(), recLayers);
        if (hLayers && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            clickedAction = 1;

        DrawRectangleRounded(recLayers, 0.25f, 4, activePopover == 1 ? Color{37, 99, 235, 200} : (hLayers ? Color{45, 55, 75, 255} : Color{25, 30, 42, 255}));
        DrawRectangleRoundedLinesEx(recLayers, 0.25f, 4, 1.2f, activePopover == 1 ? COLOR_CYAN : PANEL_BORDER);
        // Draw stacked layers symbol
        drawText("LYR", recLayers.x + 9.0f, recLayers.y + 14.0f, 12.0f, activePopover == 1 ? WHITE : (hLayers ? TEXT_PRIMARY : COLOR_CYAN));

        // 2. Settings Gear Icon
        Rectangle recSettings = {bounds.x + (bounds.width - btnSize) / 2.0f, startY + (btnSize + gap), btnSize, btnSize};
        bool hSettings = CheckCollisionPointRec(GetMousePosition(), recSettings);
        if (hSettings && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            clickedAction = 2;

        DrawRectangleRounded(recSettings, 0.25f, 4, activePopover == 2 ? Color{37, 99, 235, 200} : (hSettings ? Color{45, 55, 75, 255} : Color{25, 30, 42, 255}));
        DrawRectangleRoundedLinesEx(recSettings, 0.25f, 4, 1.2f, activePopover == 2 ? COLOR_CYAN : PANEL_BORDER);
        drawText("CFG", recSettings.x + 9.0f, recSettings.y + 14.0f, 12.0f, activePopover == 2 ? WHITE : (hSettings ? TEXT_PRIMARY : TEXT_MUTED));

        // 3. Alerts Bell Icon (with badge)
        Rectangle recAlerts = {bounds.x + (bounds.width - btnSize) / 2.0f, startY + (btnSize + gap) * 2.0f, btnSize, btnSize};
        bool hAlerts = CheckCollisionPointRec(GetMousePosition(), recAlerts);
        if (hAlerts && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            clickedAction = 3;

        DrawRectangleRounded(recAlerts, 0.25f, 4, activePopover == 3 ? Color{37, 99, 235, 200} : (hAlerts ? Color{45, 55, 75, 255} : Color{25, 30, 42, 255}));
        DrawRectangleRoundedLinesEx(recAlerts, 0.25f, 4, 1.2f, activePopover == 3 ? COLOR_GOLD : PANEL_BORDER);
        drawText("ALT", recAlerts.x + 9.0f, recAlerts.y + 14.0f, 12.0f, alertCount > 0 ? COLOR_GOLD : (activePopover == 3 ? WHITE : TEXT_MUTED));

        // Badge count for alerts
        if (alertCount > 0)
        {
            char bBuf[16];
            std::snprintf(bBuf, sizeof(bBuf), "%d", alertCount);
            DrawCircle(static_cast<int>(recAlerts.x + recAlerts.width - 4.0f), static_cast<int>(recAlerts.y + 4.0f), 8.0f, COLOR_RED);
            int tw = measureText(bBuf, 10.0f);
            drawText(bBuf, recAlerts.x + recAlerts.width - 4.0f - tw * 0.5f, recAlerts.y - 2.0f, 10.0f, WHITE);
        }

        return clickedAction;
    }

    bool UIComponents::drawTabButton(Rectangle bounds, const char *text, bool active)
    {
        bool hovered = CheckCollisionPointRec(GetMousePosition(), bounds);
        bool clicked = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

        Color bg = active ? COLOR_CYAN : (hovered ? Color{45, 55, 75, 255} : BLANK);
        Color textCol = active ? WHITE : TEXT_MUTED;

        DrawRectangleRounded(bounds, 0.2f, 4, bg);

        int w = measureText(text, 13.0f);
        drawText(text, bounds.x + (bounds.width - w) / 2.0f, bounds.y + (bounds.height - 13.0f) / 2.0f, 13.0f, textCol);

        return clicked;
    }

    int UIComponents::drawStepControls(Rectangle bounds, bool isPlaying, int currentStep, int totalSteps)
    {
        int action = 0; // 1=prev, 2=play, 3=next, 4=reset
        float btnSize = 28.0f;
        float gap = 6.0f;
        float startX = bounds.x;
        float centerY = bounds.y + bounds.height / 2.0f;

        // Reset button ⏮
        Rectangle rReset = {startX, centerY - btnSize / 2.0f, btnSize, btnSize};
        if (drawButton(rReset, "|<"))
            action = 4;
        startX += btnSize + gap;

        // Prev step ◀
        Rectangle rPrev = {startX, centerY - btnSize / 2.0f, btnSize, btnSize};
        if (drawButton(rPrev, "<"))
            action = 1;
        startX += btnSize + gap;

        // Play / Pause auto-step ▶ / ||
        Rectangle rPlay = {startX, centerY - btnSize / 2.0f, btnSize, btnSize};
        if (drawButton(rPlay, isPlaying ? "||" : ">", isPlaying))
            action = 2;
        startX += btnSize + gap;

        // Next step ⏭
        Rectangle rNext = {startX, centerY - btnSize / 2.0f, btnSize, btnSize};
        if (drawButton(rNext, ">|"))
            action = 3;
        startX += btnSize + gap + 4.0f;

        // Step counter text: e.g. "Step 14/42"
        char stepBuf[32];
        if (totalSteps > 0)
        {
            std::snprintf(stepBuf, sizeof(stepBuf), "Step %d/%d", currentStep + 1, totalSteps);
        }
        else
        {
            std::snprintf(stepBuf, sizeof(stepBuf), "Step 0/0");
        }
        drawText(stepBuf, startX, centerY - 6.0f, 12.0f, COLOR_CYAN);

        return action;
    }

    float UIComponents::drawTimelineScrubber(Rectangle bounds, float normalizedPos, bool isPaused)
    {
        float newPos = normalizedPos;
        float centerY = bounds.y + bounds.height / 2.0f;
        float startX = bounds.x;

        drawText("timeline", startX, centerY - 6.0f, 12.0f, TEXT_MUTED);
        int labelW = measureText("timeline", 12.0f);
        startX += labelW + 10.0f;

        float trackWidth = (bounds.x + bounds.width) - startX - 8.0f;
        float trackX = startX;

        DrawLineEx({trackX, centerY}, {trackX + trackWidth, centerY}, 3.0f, PANEL_BORDER);

        float knobX = trackX + normalizedPos * trackWidth;
        Rectangle trackArea = {trackX, centerY - 12.0f, trackWidth, 24.0f};

        if (CheckCollisionPointRec(GetMousePosition(), trackArea) && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            float mouseX = GetMousePosition().x;
            newPos = (mouseX - trackX) / trackWidth;
            newPos = std::clamp(newPos, 0.0f, 1.0f);
            knobX = trackX + newPos * trackWidth;
        }

        DrawCircle(static_cast<int>(knobX), static_cast<int>(centerY), 7.0f, COLOR_CYAN);
        DrawCircleLines(static_cast<int>(knobX), static_cast<int>(centerY), 8.0f, WHITE);

        return newPos;
    }

    bool UIComponents::drawDisruptionButton(Rectangle bounds, const char *icon, const char *label, bool active)
    {
        bool hovered = CheckCollisionPointRec(GetMousePosition(), bounds);
        bool clicked = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

        Color bg = active ? Color{50, 20, 20, 255} : (hovered ? Color{35, 40, 52, 255} : Color{20, 24, 34, 255});
        Color border = active ? COLOR_RED : (hovered ? COLOR_GOLD : PANEL_BORDER);

        DrawRectangleRounded(bounds, 0.15f, 4, bg);
        DrawRectangleRoundedLinesEx(bounds, 0.15f, 4, active ? 2.0f : 1.2f, border);

        drawText(icon, bounds.x + 8.0f, bounds.y + (bounds.height - 14.0f) / 2.0f, 14.0f, active ? COLOR_RED : COLOR_GOLD);
        drawText(label, bounds.x + 28.0f, bounds.y + (bounds.height - 12.0f) / 2.0f, 12.0f, active ? WHITE : TEXT_PRIMARY);

        return clicked;
    }

    float UIComponents::drawSlider(Rectangle bounds, const char *label, float value, float minVal, float maxVal)
    {
        float newPos = value;
        drawText(label, bounds.x, bounds.y, 12.0f, TEXT_MUTED);

        char valStr[32];
        std::snprintf(valStr, sizeof(valStr), "%.2f", value);
        int valW = measureText(valStr, 12.0f);
        drawText(valStr, bounds.x + bounds.width - valW, bounds.y, 12.0f, TEXT_PRIMARY);

        float trackY = bounds.y + 20.0f;
        DrawLineEx({bounds.x, trackY}, {bounds.x + bounds.width, trackY}, 2.0f, PANEL_BORDER);

        float norm = (value - minVal) / (maxVal - minVal);
        norm = std::clamp(norm, 0.0f, 1.0f);
        float knobX = bounds.x + norm * bounds.width;

        Rectangle trackArea = {bounds.x, trackY - 10.0f, bounds.width, 20.0f};
        if (CheckCollisionPointRec(GetMousePosition(), trackArea) && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            float mouseX = GetMousePosition().x;
            float newNorm = (mouseX - bounds.x) / bounds.width;
            newNorm = std::clamp(newNorm, 0.0f, 1.0f);
            newPos = minVal + newNorm * (maxVal - minVal);
            knobX = bounds.x + newNorm * bounds.width;
        }

        DrawCircle(static_cast<int>(knobX), static_cast<int>(trackY), 6.0f, COLOR_CYAN);
        return newPos;
    }

} // namespace dhaka
