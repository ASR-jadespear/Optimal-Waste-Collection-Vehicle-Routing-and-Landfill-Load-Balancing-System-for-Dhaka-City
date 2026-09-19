#include "ui/Renderer.hpp"
#include <raymath.h>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace dhaka
{

    Renderer::Renderer(SimulationEngine &eng, int screenWidth, int screenHeight)
        : engine(eng), camera(screenWidth, screenHeight)
    {
        UIComponents::initFont();
        engine.generateTraceForStage(0);
    }

    // ─────────────────────────────────────────────────────────────
    //  LAYOUT COMPUTATION
    // ─────────────────────────────────────────────────────────────

    void Renderer::computeLayout()
    {
        float w = static_cast<float>(GetScreenWidth());
        float h = static_cast<float>(GetScreenHeight());

        float titleH = 56.0f;
        float stepperH = 48.0f;
        float bottomH = 72.0f;
        float iconRailW = 64.0f;
        float rightPanelW = std::max(290.0f, w * 0.28f);

        titleBarRect = {0, 0, w, titleH};
        stepperRect = {0, titleH, w, stepperH};

        float bodyTop = titleH + stepperH;
        float bodyH = h - bodyTop - bottomH;

        iconRailRect = {0, bodyTop, iconRailW, bodyH};
        rightPanelRect = {w - rightPanelW, bodyTop, rightPanelW, bodyH};
        mapRect = {iconRailW, bodyTop, w - iconRailW - rightPanelW, bodyH};
        bottomBarRect = {0, h - bottomH, w, bottomH};
    }

    // ─────────────────────────────────────────────────────────────
    //  UPDATE
    // ─────────────────────────────────────────────────────────────

    void Renderer::update(float dt)
    {
        computeLayout();
        animWaveTimer += dt;
        engine.activeTrace.update(dt);

        // Pan and zoom camera only when mouse is over the map area
        Vector2 mouseScreen = GetMousePosition();
        if (CheckCollisionPointRec(mouseScreen, mapRect))
        {
            camera.update(dt);
        }

        handleInteractions();

        // Keyboard shortcuts
        if (IsKeyPressed(KEY_SPACE))
            engine.isPaused = !engine.isPaused;
        if (IsKeyPressed(KEY_ONE))
        {
            activeStage = PipelineStage::ROAD_NETWORK;
            engine.generateTraceForStage(0);
        }
        if (IsKeyPressed(KEY_TWO))
        {
            activeStage = PipelineStage::ROUTING;
            engine.generateTraceForStage(1);
        }
        if (IsKeyPressed(KEY_THREE))
        {
            activeStage = PipelineStage::SEQUENCING;
            engine.generateTraceForStage(2);
        }
        if (IsKeyPressed(KEY_FOUR))
        {
            activeStage = PipelineStage::LOAD_SELECT;
            engine.generateTraceForStage(3);
        }
        if (IsKeyPressed(KEY_FIVE))
        {
            activeStage = PipelineStage::LANDFILL_BALANCE;
            engine.generateTraceForStage(4);
        }
        if (IsKeyPressed(KEY_R))
            engine.triggerRandomDhakaDisruption();

        // Speed controls
        if (IsKeyPressed(KEY_LEFT_BRACKET))
            engine.simSpeedMultiplier = std::max(0.5f, engine.simSpeedMultiplier * 0.5f);
        if (IsKeyPressed(KEY_RIGHT_BRACKET))
            engine.simSpeedMultiplier = std::min(20.0f, engine.simSpeedMultiplier * 2.0f);

        // Update timeline scrubber position from simulation time (modulo 24 hrs)
        double totalSimHours = 24.0;
        timelinePos = static_cast<float>(std::fmod(engine.getSimTimeHours(), totalSimHours) / totalSimHours);
    }

    // ─────────────────────────────────────────────────────────────
    //  INTERACTIONS
    // ─────────────────────────────────────────────────────────────

    void Renderer::handleInteractions()
    {
        Vector2 mouseScreen = GetMousePosition();
        Vec2 mouseWorld = camera.screenToWorld(mouseScreen);

        bool overMap = CheckCollisionPointRec(mouseScreen, mapRect);

        hoveredBinId = -1;
        hoveredEdgeId = -1;
        hoveredVehicleId = -1;
        hoveredLandfillId = -1;

        // If inline editor or popovers are active and mouse is inside them, suppress map interactions
        if (roadEditor.active || binEditor.active)
            return;
        if (activePopover != 0 && CheckCollisionPointRec(mouseScreen, {iconRailRect.x + iconRailRect.width, iconRailRect.y, 220.0f, 260.0f}))
            return;

        if (!overMap)
            return;

        // 1. Check bin hover & click
        for (const auto &bin : engine.data.bins)
        {
            float dist = mouseWorld.distanceTo(bin.position);
            float radius = 8.0f + static_cast<float>(bin.getFillRatio() * 12.0f);
            if (dist <= radius + 6.0f)
            {
                hoveredBinId = bin.id;
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                {
                    if (disruptionMode == 3)
                    {
                        engine.triggerBinOverflow(bin.id, 750.0);
                        disruptionMode = 0;
                    }
                    else
                    {
                        // Open inline bin editor
                        binEditor.active = true;
                        binEditor.binId = bin.id;
                        binEditor.worldPos = bin.position;
                        binEditor.screenPos = mouseScreen;
                        binEditor.capacityKg = static_cast<float>(bin.capacityKg);
                        binEditor.initialWasteKg = static_cast<float>(bin.currentWasteKg);
                    }
                }
                break;
            }
        }

        // 2. Check vehicle hover
        if (hoveredBinId == -1)
        {
            for (const auto &v : engine.data.vehicles)
            {
                float dist = mouseWorld.distanceTo(v.currentPosition);
                if (dist <= 16.0f)
                {
                    hoveredVehicleId = v.id;
                    break;
                }
            }
        }

        // 3. Check landfill hover
        if (hoveredBinId == -1 && hoveredVehicleId == -1)
        {
            for (const auto &lf : engine.data.landfills)
            {
                float dist = mouseWorld.distanceTo(lf.position);
                if (dist <= 30.0f)
                {
                    hoveredLandfillId = lf.id;
                    break;
                }
            }
        }

        // 4. Check road hover & click
        if (hoveredBinId == -1 && hoveredVehicleId == -1 && hoveredLandfillId == -1)
        {
            int edgeId = engine.data.graph.findNearestEdge(mouseWorld, 15.0f);
            if (edgeId >= 0)
            {
                hoveredEdgeId = edgeId;
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                {
                    if (disruptionMode == 1)
                    {
                        engine.triggerRoadClosure(edgeId);
                        disruptionMode = 0;
                    }
                    else if (disruptionMode == 2)
                    {
                        engine.triggerRoadCongestion(edgeId, 4.5);
                        disruptionMode = 0;
                    }
                    else
                    {
                        // Open inline road weight editor right at click point!
                        const auto &edge = engine.data.graph.getEdge(edgeId);
                        roadEditor.active = true;
                        roadEditor.edgeId = edgeId;
                        roadEditor.screenPos = mouseScreen;
                        roadEditor.tempSpeed = static_cast<float>(edge.baseSpeedKmh);
                        roadEditor.tempCongestion = static_cast<float>(edge.congestionFactor);
                        roadEditor.tempClosed = edge.isClosed;
                    }
                }
            }
            else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && disruptionMode == 0)
            {
                // Click on empty map space -> Open inline editor to create a new bin!
                binEditor.active = true;
                binEditor.binId = -1; // New bin
                binEditor.worldPos = mouseWorld;
                binEditor.screenPos = mouseScreen;
                binEditor.capacityKg = 1500.0f;
                binEditor.initialWasteKg = 600.0f;
            }
        }
    }

    // ─────────────────────────────────────────────────────────────
    //  MAIN RENDER
    // ─────────────────────────────────────────────────────────────

    void Renderer::render()
    {
        BeginDrawing();
        ClearBackground(UIComponents::BG_DARK);

        // 1. Title Bar
        bool pauseClicked = UIComponents::drawTitleBar(titleBarRect, engine.isPaused,
                                                       engine.simTimeSeconds, engine.simSpeedMultiplier, engine.isLiveMode);
        if (pauseClicked)
            engine.isPaused = !engine.isPaused;

        // 2. Pipeline Stepper
        int clickedStage = UIComponents::drawPipelineStepper(stepperRect, static_cast<int>(activeStage));
        if (clickedStage >= 0 && clickedStage <= 4)
        {
            activeStage = static_cast<PipelineStage>(clickedStage);
            engine.generateTraceForStage(clickedStage);
        }

        // 3. Map Region (camera-transformed)
        BeginScissorMode(static_cast<int>(mapRect.x), static_cast<int>(mapRect.y),
                         static_cast<int>(mapRect.width), static_cast<int>(mapRect.height));
        BeginMode2D(camera.camera);
        renderBaseMap();
        renderStageOverlay();
        EndMode2D();
        EndScissorMode();

        // 4. Icon Rail
        int railAction = UIComponents::drawIconRail(iconRailRect, layerFlags,
                                                    static_cast<int>(engine.activeDisruptions.size()), activePopover);
        if (railAction > 0)
        {
            activePopover = (activePopover == railAction) ? 0 : railAction;
        }

        // 5. Rail Popovers (Layers / Settings / Alerts)
        renderRailPopovers();

        // 6. Right Panel
        renderRightPanel();

        // 7. Bottom Bar Step Controls & Scrubber
        DrawRectangleRec(bottomBarRect, UIComponents::PANEL_BG);
        DrawLineEx({bottomBarRect.x, bottomBarRect.y},
                   {bottomBarRect.x + bottomBarRect.width, bottomBarRect.y}, 1.0f, UIComponents::PANEL_BORDER);

        // Step-through controls ⏮ ◀ ▶ ⏭ on bottom left
        Rectangle stepCtrlBounds = {bottomBarRect.x + 16.0f, bottomBarRect.y + 12.0f, 240.0f, 32.0f};
        int stepAction = UIComponents::drawStepControls(stepCtrlBounds, engine.activeTrace.isPlaying,
                                                        engine.activeTrace.currentStepIndex,
                                                        static_cast<int>(engine.activeTrace.steps.size()));
        if (stepAction == 1)
            engine.activeTrace.stepBackward();
        else if (stepAction == 2)
            engine.activeTrace.isPlaying = !engine.activeTrace.isPlaying;
        else if (stepAction == 3)
            engine.activeTrace.stepForward();
        else if (stepAction == 4)
            engine.activeTrace.resetToStart();

        // Timeline scrubber in middle
        float scrubX = bottomBarRect.x + 280.0f;
        float scrubW = bottomBarRect.width - 280.0f - 490.0f;
        Rectangle scrubBounds = {scrubX, bottomBarRect.y + 14.0f, scrubW, 28.0f};

        float newTimelinePos = UIComponents::drawTimelineScrubber(scrubBounds, timelinePos, engine.isPaused);
        if (std::abs(newTimelinePos - timelinePos) > 0.005f)
        {
            timelinePos = newTimelinePos;
            engine.setSimTimeHours(timelinePos * 24.0);
        }

        // Disruption buttons on right
        float dbX = bottomBarRect.x + bottomBarRect.width - 470.0f;
        float dbY = bottomBarRect.y + 14.0f;
        float dbW = 145.0f;
        float dbH = 34.0f;

        if (UIComponents::drawDisruptionButton({dbX, dbY, dbW, dbH}, "/\\", "Close road", disruptionMode == 1))
        {
            disruptionMode = (disruptionMode == 1) ? 0 : 1;
        }
        if (UIComponents::drawDisruptionButton({dbX + dbW + 8.0f, dbY, dbW, dbH}, "!!", "Spike congestion", disruptionMode == 2))
        {
            disruptionMode = (disruptionMode == 2) ? 0 : 2;
        }
        if (UIComponents::drawDisruptionButton({dbX + (dbW + 8.0f) * 2.0f, dbY, dbW, dbH}, "+", "New overflow", disruptionMode == 3))
        {
            disruptionMode = (disruptionMode == 3) ? 0 : 3;
        }

        // Bottom Narration Banner: current step explanation or simulation status
        const AlgorithmStep *currentStep = engine.activeTrace.getCurrentStep();
        std::string bottomNarration = currentStep ? currentStep->narration : engine.lastAlgorithmStatusMessage;
        if (!bottomNarration.empty())
        {
            int maxChars = static_cast<int>((bottomBarRect.width - 32.0f) / 7.0f);
            if (bottomNarration.length() > static_cast<size_t>(maxChars))
            {
                bottomNarration = bottomNarration.substr(0, maxChars - 3) + "...";
            }
            UIComponents::drawText(bottomNarration.c_str(), bottomBarRect.x + 16.0f, bottomBarRect.y + 50.0f, 11.0f, UIComponents::COLOR_GOLD);
        }

        // 8. Disruption Mode Banner
        if (disruptionMode > 0)
        {
            const char *modeText = disruptionMode == 1 ? "Click any road edge to CLOSE it" : (disruptionMode == 2 ? "Click any road edge to CONGEST it (4.5x)" : "Click any bin to trigger 750kg OVERFLOW");
            int tw = UIComponents::measureText(modeText, 13.0f);
            DrawRectangle(static_cast<int>(mapRect.x + mapRect.width * 0.5f - tw * 0.5f - 14),
                          static_cast<int>(mapRect.y + 12), tw + 28, 30, {220, 38, 38, 220});
            UIComponents::drawText(modeText,
                                   mapRect.x + mapRect.width * 0.5f - tw * 0.5f,
                                   mapRect.y + 20.0f, 13.0f, WHITE);
        }

        // 9. Inline Map Editors (Road Editor / Bin Editor)
        renderInlineRoadEditor();
        renderInlineBinEditor();

        // 10. Tooltips (on top)
        renderTooltips();

        EndDrawing();
    }

    // ─────────────────────────────────────────────────────────────
    //  BASE MAP RENDERING
    // ─────────────────────────────────────────────────────────────

    void Renderer::renderBaseMap()
    {
        if (layerFlags[3]) // water
            renderWaterways();
        if (layerFlags[0]) // roads
            renderRoads();
        renderActivePaths();
        if (layerFlags[1]) // bins
            renderBins();
        renderLandfills();
        renderDepots();
        if (layerFlags[2]) // trucks
            renderVehicles();
    }

    void Renderer::renderWaterways()
    {
        Color waterColor = {24, 44, 68, 200};
        for (const auto &waterway : engine.data.waterways)
        {
            if (waterway.size() < 2)
                continue;
            for (size_t i = 0; i + 1 < waterway.size(); ++i)
            {
                Vector2 p1 = {waterway[i].x, waterway[i].y};
                Vector2 p2 = {waterway[i + 1].x, waterway[i + 1].y};
                DrawLineEx(p1, p2, 14.0f, waterColor);
            }
        }

        for (const auto &[label, pos] : engine.data.districtLabels)
        {
            UIComponents::drawText(label.c_str(), pos.x - 60.0f, pos.y, 16.0f, {40, 50, 70, 160});
        }
    }

    void Renderer::renderRoads()
    {
        double hourOfDay = engine.getHourOfDay();

        for (const auto &edge : engine.data.graph.edges)
        {
            if (edge.fromNode > edge.toNode)
                continue;

            const auto &nA = engine.data.graph.getNode(edge.fromNode);
            const auto &nB = engine.data.graph.getNode(edge.toNode);

            Vector2 pA = {nA.position.x, nA.position.y};
            Vector2 pB = {nB.position.x, nB.position.y};

            if (edge.isClosed)
            {
                // USER SPEC: "a closed road shows dashed gray"
                UIComponents::drawDashedLine(pA, pB, 4.0f, 10.0f, UIComponents::COLOR_GRAY);

                // Roadblock marker (X) at midpoint
                Vector2 mid = {(pA.x + pB.x) * 0.5f, (pA.y + pB.y) * 0.5f};
                DrawCircle(static_cast<int>(mid.x), static_cast<int>(mid.y), 7.0f, {50, 50, 60, 255});
                DrawLine(static_cast<int>(mid.x - 4), static_cast<int>(mid.y - 4),
                         static_cast<int>(mid.x + 4), static_cast<int>(mid.y + 4), UIComponents::COLOR_RED);
                DrawLine(static_cast<int>(mid.x - 4), static_cast<int>(mid.y + 4),
                         static_cast<int>(mid.x + 4), static_cast<int>(mid.y - 4), UIComponents::COLOR_RED);
            }
            else
            {
                // Roads color-scaled green -> amber -> red by total congestion factor
                double totalCongestion = edge.congestionFactor * edge.getTimeOfDayMultiplier(hourOfDay);

                Color roadColor;
                float roadThickness = 3.5f;

                if (totalCongestion >= 3.0)
                {
                    roadColor = UIComponents::COLOR_RED;
                    roadThickness = 4.5f;
                }
                else if (totalCongestion > 1.4)
                {
                    roadColor = UIComponents::COLOR_GOLD;
                    roadThickness = 4.0f;
                }
                else
                {
                    roadColor = Color{46, 75, 60, 220};
                    roadThickness = 2.8f;
                }

                if (edge.id == hoveredEdgeId)
                {
                    roadThickness += 2.0f;
                    roadColor = UIComponents::COLOR_CYAN;
                }

                DrawLineEx(pA, pB, roadThickness, roadColor);
            }
        }
    }

    void Renderer::renderActivePaths()
    {
        for (const auto &v : engine.data.vehicles)
        {
            if (v.pathNodeIds.size() < 2)
                continue;

            Color pathColor = (v.corporation == Corporation::DNCC)
                                  ? ColorAlpha(UIComponents::COLOR_CYAN, 0.55f)
                                  : ColorAlpha(UIComponents::COLOR_GOLD, 0.55f);

            for (size_t i = v.currentPathSegmentIndex; i + 1 < v.pathNodeIds.size(); ++i)
            {
                int u = v.pathNodeIds[i];
                int next = v.pathNodeIds[i + 1];

                Vector2 p1 = (static_cast<int>(i) == v.currentPathSegmentIndex)
                                 ? Vector2{v.currentPosition.x, v.currentPosition.y}
                                 : Vector2{engine.data.graph.getNode(u).position.x,
                                           engine.data.graph.getNode(u).position.y};

                Vector2 p2 = {engine.data.graph.getNode(next).position.x,
                              engine.data.graph.getNode(next).position.y};

                DrawLineEx(p1, p2, 2.5f, pathColor);
            }
        }
    }

    void Renderer::renderBins()
    {
        float time = static_cast<float>(GetTime());

        for (const auto &bin : engine.data.bins)
        {
            float fillRatio = static_cast<float>(bin.getFillRatio());
            float baseRadius = 6.0f + std::min(14.0f, fillRatio * 10.0f);

            Color binColor;
            if (bin.isOverflowing())
                binColor = UIComponents::COLOR_RED;
            else if (fillRatio >= 0.75f)
                binColor = UIComponents::COLOR_GOLD;
            else
                binColor = UIComponents::COLOR_GREEN;

            // Pulsing alert ring for overflowing bins
            if (bin.isOverflowing())
            {
                float pulse = (std::sin(time * 8.0f) + 1.0f) * 0.5f;
                float pulseRadius = baseRadius + 4.0f + pulse * 7.0f;
                DrawCircleLines(static_cast<int>(bin.position.x), static_cast<int>(bin.position.y),
                                pulseRadius, ColorAlpha(UIComponents::COLOR_RED, 0.7f - pulse * 0.4f));
            }

            DrawCircle(static_cast<int>(bin.position.x), static_cast<int>(bin.position.y),
                       baseRadius + 2.0f, {20, 24, 34, 220});
            DrawCircle(static_cast<int>(bin.position.x), static_cast<int>(bin.position.y),
                       baseRadius, binColor);

            if (bin.id == hoveredBinId)
            {
                DrawCircleLines(static_cast<int>(bin.position.x), static_cast<int>(bin.position.y),
                                baseRadius + 4.0f, UIComponents::COLOR_CYAN);
            }

            // Labels when zoomed in enough
            if (camera.camera.zoom >= 0.75f)
            {
                UIComponents::drawText(bin.name.c_str(),
                                       bin.position.x - 20.0f,
                                       bin.position.y + baseRadius + 3.0f,
                                       10.0f, UIComponents::TEXT_MUTED);
            }
        }
    }

    void Renderer::renderLandfills()
    {
        float time = static_cast<float>(GetTime());

        for (const auto &lf : engine.data.landfills)
        {
            Vector2 pos = {lf.position.x, lf.position.y};

            // USER SPEC: "Landfills as two fixed icons, each with a radial fill showing % of daily intake used."
            float intakePct = static_cast<float>(lf.getIntakePercentage());
            Color gaugeColor = (intakePct > 85.0f)   ? UIComponents::COLOR_RED
                               : (intakePct > 60.0f) ? UIComponents::COLOR_GOLD
                                                     : (lf.id == 0 ? UIComponents::COLOR_CYAN : UIComponents::COLOR_PURPLE);

            // Flashing ring if binding / saturated
            if (intakePct > 80.0f)
            {
                float pulse = (std::sin(time * 6.0f) + 1.0f) * 0.5f;
                DrawCircleLines(static_cast<int>(pos.x), static_cast<int>(pos.y),
                                26.0f + pulse * 6.0f, ColorAlpha(UIComponents::COLOR_RED, 0.8f - pulse * 0.4f));
            }

            // Radial gauge ring around the icon
            UIComponents::drawRadialGauge(pos, 18.0f, 24.0f, intakePct, gaugeColor, {30, 36, 50, 180});

            // Diamond Icon Body
            DrawRectanglePro({pos.x, pos.y, 28.0f, 28.0f}, {14.0f, 14.0f}, 45.0f, {20, 26, 38, 255});
            DrawRectangleLinesEx({pos.x - 14.0f, pos.y - 14.0f, 28.0f, 28.0f}, 1.5f, gaugeColor);

            // Landfill short name
            UIComponents::drawText(lf.shortName.c_str(), pos.x - 28.0f, pos.y - 34.0f,
                                   12.0f, UIComponents::TEXT_PRIMARY);

            // Percentage label inside
            char pBuf[16];
            std::snprintf(pBuf, sizeof(pBuf), "%.0f%%", intakePct);
            int tw = UIComponents::measureText(pBuf, 10.0f);
            UIComponents::drawText(pBuf, pos.x - tw * 0.5f, pos.y - 5.0f, 10.0f, WHITE);

            // Queue count indicator badge
            if (lf.queueCount > 0)
            {
                DrawCircle(static_cast<int>(pos.x + 20.0f), static_cast<int>(pos.y - 18.0f), 8.0f, UIComponents::COLOR_GOLD);
                char qBuf[8];
                std::snprintf(qBuf, sizeof(qBuf), "%d", lf.queueCount);
                UIComponents::drawText(qBuf, pos.x + 17.0f, pos.y - 23.0f, 10.0f, BLACK);
            }
        }
    }

    void Renderer::renderDepots()
    {
        int nUttara = 2;
        int nDholai = 3;

        for (int nodeId : {nUttara, nDholai})
        {
            if (nodeId >= engine.data.graph.getNodeCount())
                continue;
            const auto &node = engine.data.graph.getNode(nodeId);
            Vector2 pos = {node.position.x, node.position.y};

            DrawRectangleRounded({pos.x - 14.0f, pos.y - 14.0f, 28.0f, 28.0f}, 0.2f, 4, {30, 41, 59, 255});
            DrawRectangleRoundedLinesEx({pos.x - 14.0f, pos.y - 14.0f, 28.0f, 28.0f}, 0.2f, 4, 1.5f, UIComponents::COLOR_CYAN);
            UIComponents::drawText("DEPOT", pos.x - 16.0f, pos.y - 4.0f, 8.0f, UIComponents::COLOR_CYAN);
        }
    }

    void Renderer::renderVehicles()
    {
        for (const auto &v : engine.data.vehicles)
        {
            Vector2 pos = {v.currentPosition.x, v.currentPosition.y};
            Color truckColor = (v.corporation == Corporation::DNCC)
                                   ? UIComponents::COLOR_CYAN
                                   : UIComponents::COLOR_GOLD;

            // Truck icon body
            Rectangle truckRec = {pos.x, pos.y, 22.0f, 13.0f};
            Vector2 origin = {11.0f, 6.5f};
            DrawRectanglePro(truckRec, origin, v.headingAngle, truckColor);
            DrawRectangleLinesEx({pos.x - 11.0f, pos.y - 7.0f, 22.0f, 14.0f}, 1.0f, WHITE);

            // USER SPEC: "capacity-fill ring around them (like a battery indicator)"
            float loadPct = static_cast<float>(v.getLoadPercentage());
            float ringRadius = 14.0f;
            Color ringColor = loadPct > 85.0f ? UIComponents::COLOR_RED : UIComponents::COLOR_GREEN;

            DrawCircleLines(static_cast<int>(pos.x), static_cast<int>(pos.y), ringRadius, {30, 40, 55, 100});
            if (loadPct > 0.0f)
            {
                DrawRing(pos, ringRadius - 2.5f, ringRadius, 0.0f, (loadPct / 100.0f) * 360.0f, 24, ringColor);
            }

            // State indicator dot
            if (v.state == VehicleState::COLLECTING_WASTE)
            {
                DrawCircle(static_cast<int>(pos.x), static_cast<int>(pos.y - 20.0f), 4.0f, UIComponents::COLOR_GOLD);
            }
            else if (v.state == VehicleState::EN_ROUTE_TO_LANDFILL)
            {
                DrawCircle(static_cast<int>(pos.x), static_cast<int>(pos.y - 20.0f), 4.0f, UIComponents::COLOR_PURPLE);
            }
        }
    }

    // ─────────────────────────────────────────────────────────────
    //  STAGE-SPECIFIC OVERLAYS
    // ─────────────────────────────────────────────────────────────

    void Renderer::renderStageOverlay()
    {
        switch (activeStage)
        {
        case PipelineStage::ROAD_NETWORK:
            renderOverlay_RoadNetwork();
            break;
        case PipelineStage::ROUTING:
            renderOverlay_Routing();
            break;
        case PipelineStage::SEQUENCING:
            renderOverlay_Sequencing();
            break;
        case PipelineStage::LOAD_SELECT:
            renderOverlay_LoadSelect();
            break;
        case PipelineStage::LANDFILL_BALANCE:
            renderOverlay_LandfillBalance();
            break;
        }
    }

    void Renderer::renderOverlay_RoadNetwork()
    {
        double hourOfDay = engine.getHourOfDay();

        for (const auto &edge : engine.data.graph.edges)
        {
            if (edge.fromNode > edge.toNode)
                continue;

            const auto &nA = engine.data.graph.getNode(edge.fromNode);
            const auto &nB = engine.data.graph.getNode(edge.toNode);

            Vector2 mid = {(nA.position.x + nB.position.x) * 0.5f,
                           (nA.position.y + nB.position.y) * 0.5f};

            // Travel time label
            char timeBuf[32];
            if (edge.isClosed)
            {
                std::snprintf(timeBuf, sizeof(timeBuf), "CLOSED");
            }
            else
            {
                std::snprintf(timeBuf, sizeof(timeBuf), "%.0fs", edge.getTravelTimeSeconds(hourOfDay));
            }
            UIComponents::drawText(timeBuf, mid.x - 12.0f, mid.y - 14.0f, 9.0f,
                                   edge.isClosed ? UIComponents::COLOR_RED : UIComponents::TEXT_MUTED);

            // Congestion factor badge
            double totalCongestion = edge.congestionFactor * edge.getTimeOfDayMultiplier(hourOfDay);
            if (!edge.isClosed && totalCongestion > 1.2)
            {
                char congBuf[16];
                std::snprintf(congBuf, sizeof(congBuf), "%.1fx", totalCongestion);
                UIComponents::drawText(congBuf, mid.x - 8.0f, mid.y + 2.0f, 8.0f,
                                       totalCongestion >= 3.0 ? UIComponents::COLOR_RED : UIComponents::COLOR_GOLD);
            }
        }

        // Node ID labels
        if (showNodeIds || camera.camera.zoom >= 0.6f)
        {
            for (int i = 0; i < engine.data.graph.getNodeCount(); ++i)
            {
                const auto &node = engine.data.graph.getNode(i);
                char nBuf[16];
                std::snprintf(nBuf, sizeof(nBuf), "%d", i);
                UIComponents::drawText(nBuf, node.position.x + 6.0f, node.position.y - 12.0f,
                                       8.0f, {80, 100, 130, 180});
                DrawCircle(static_cast<int>(node.position.x), static_cast<int>(node.position.y),
                           3.0f, {80, 100, 130, 150});
            }
        }
    }

    void Renderer::renderOverlay_Routing()
    {
        float time = static_cast<float>(GetTime());
        const AlgorithmStep *step = engine.activeTrace.getCurrentStep();

        // USER SPEC: "draws the animated Dijkstra/A* frontier and the settled path"
        if (step && step->type == TraceType::ROUTING_ASTAR)
        {
            // 1. Draw frontier wavefront nodes (pulsing circles)
            for (int nodeId : step->frontierNodeIds)
            {
                if (nodeId < 0 || nodeId >= engine.data.graph.getNodeCount())
                    continue;
                const auto &node = engine.data.graph.getNode(nodeId);
                float pulseR = 10.0f + (std::sin(animWaveTimer * 6.0f) + 1.0f) * 6.0f;
                DrawCircleLines(static_cast<int>(node.position.x), static_cast<int>(node.position.y),
                                pulseR, UIComponents::COLOR_CYAN);
            }

            // 2. Draw settled/explored nodes
            for (int nodeId : step->settledNodeIds)
            {
                if (nodeId < 0 || nodeId >= engine.data.graph.getNodeCount())
                    continue;
                const auto &node = engine.data.graph.getNode(nodeId);
                DrawCircle(static_cast<int>(node.position.x), static_cast<int>(node.position.y), 5.0f, {37, 99, 235, 180});
            }

            // 3. Draw active edge being relaxed
            if (step->activeEdgeId >= 0 && step->activeEdgeId < engine.data.graph.getEdgeCount())
            {
                const auto &edge = engine.data.graph.getEdge(step->activeEdgeId);
                const auto &nA = engine.data.graph.getNode(edge.fromNode);
                const auto &nB = engine.data.graph.getNode(edge.toNode);
                DrawLineEx({nA.position.x, nA.position.y}, {nB.position.x, nB.position.y}, 6.0f, UIComponents::COLOR_GOLD);
            }

            // 4. Draw settled path if found
            if (!step->settledNodeIds.empty() && !step->currentPathNodeIds.empty())
            {
                for (size_t i = 0; i + 1 < step->currentPathNodeIds.size(); ++i)
                {
                    int u = step->currentPathNodeIds[i];
                    int v = step->currentPathNodeIds[i + 1];
                    const auto &nA = engine.data.graph.getNode(u);
                    const auto &nB = engine.data.graph.getNode(v);
                    DrawLineEx({nA.position.x, nA.position.y}, {nB.position.x, nB.position.y}, 5.0f, UIComponents::COLOR_GREEN);
                }
            }
        }
        else
        {
            // Real-time active paths when not in step trace
            for (const auto &v : engine.data.vehicles)
            {
                if (v.pathNodeIds.size() < 2)
                    continue;

                Color pathColor = (v.corporation == Corporation::DNCC)
                                      ? UIComponents::COLOR_CYAN
                                      : UIComponents::COLOR_GOLD;

                for (size_t i = v.currentPathSegmentIndex; i + 1 < v.pathNodeIds.size(); ++i)
                {
                    int u = v.pathNodeIds[i];
                    int next = v.pathNodeIds[i + 1];

                    Vector2 p1 = (static_cast<int>(i) == v.currentPathSegmentIndex)
                                     ? Vector2{v.currentPosition.x, v.currentPosition.y}
                                     : Vector2{engine.data.graph.getNode(u).position.x,
                                               engine.data.graph.getNode(u).position.y};
                    Vector2 p2 = {engine.data.graph.getNode(next).position.x,
                                  engine.data.graph.getNode(next).position.y};

                    float pulse = (std::sin(time * 4.0f + static_cast<float>(i) * 0.5f) + 1.0f) * 0.15f;
                    DrawLineEx(p1, p2, 4.5f, ColorAlpha(pathColor, 0.7f + pulse));
                }

                // A* marker at target destination
                int destNode = v.pathNodeIds.back();
                const auto &dest = engine.data.graph.getNode(destNode);
                DrawCircleLines(static_cast<int>(dest.position.x), static_cast<int>(dest.position.y),
                                12.0f, pathColor);
                UIComponents::drawText("A*", v.currentPosition.x + 14.0f,
                                       v.currentPosition.y - 22.0f, 10.0f, pathColor);
            }
        }
    }

    void Renderer::renderOverlay_Sequencing()
    {
        // USER SPEC: "Sequencing stage numbers the bins a truck will hit in order"
        const AlgorithmStep *step = engine.activeTrace.getCurrentStep();

        if (step && step->type == TraceType::SEQUENCING_GREEDY)
        {
            for (size_t i = 0; i < step->tourBinIdsSoFar.size(); ++i)
            {
                int binId = step->tourBinIdsSoFar[i];
                if (binId < 0 || binId >= static_cast<int>(engine.data.bins.size()))
                    continue;

                const auto &bin = engine.data.bins[binId];
                char numBuf[8];
                std::snprintf(numBuf, sizeof(numBuf), "%d", static_cast<int>(i + 1));

                float badgeX = bin.position.x + 10.0f;
                float badgeY = bin.position.y - 16.0f;

                DrawCircle(static_cast<int>(badgeX), static_cast<int>(badgeY), 10.0f, UIComponents::COLOR_GOLD);
                int tw = UIComponents::measureText(numBuf, 10.0f);
                UIComponents::drawText(numBuf, badgeX - tw * 0.5f, badgeY - 5.0f, 10.0f, WHITE);
            }

            // Connecting greedy tour lines
            for (size_t i = 0; i + 1 < step->tourBinIdsSoFar.size(); ++i)
            {
                int bA = step->tourBinIdsSoFar[i];
                int bB = step->tourBinIdsSoFar[i + 1];
                const auto &a = engine.data.bins[bA];
                const auto &b = engine.data.bins[bB];
                DrawLineEx({a.position.x, a.position.y}, {b.position.x, b.position.y}, 3.0f, UIComponents::COLOR_GOLD);
            }
        }
        else
        {
            // Show current tour for each truck
            for (const auto &v : engine.data.vehicles)
            {
                if (v.assignedBinIds.empty())
                    continue;

                Color tourColor = (v.corporation == Corporation::DNCC)
                                      ? UIComponents::COLOR_CYAN
                                      : UIComponents::COLOR_GOLD;

                for (size_t i = 0; i < v.assignedBinIds.size(); ++i)
                {
                    int binId = v.assignedBinIds[i];
                    if (binId < 0 || binId >= static_cast<int>(engine.data.bins.size()))
                        continue;

                    const auto &bin = engine.data.bins[binId];
                    char numBuf[8];
                    std::snprintf(numBuf, sizeof(numBuf), "%d", static_cast<int>(i + 1));

                    float badgeX = bin.position.x + 10.0f;
                    float badgeY = bin.position.y - 16.0f;

                    DrawCircle(static_cast<int>(badgeX), static_cast<int>(badgeY), 10.0f, tourColor);
                    int tw = UIComponents::measureText(numBuf, 10.0f);
                    UIComponents::drawText(numBuf, badgeX - tw * 0.5f, badgeY - 5.0f, 10.0f, WHITE);
                }

                for (size_t i = 0; i + 1 < v.assignedBinIds.size(); ++i)
                {
                    int bA = v.assignedBinIds[i];
                    int bB = v.assignedBinIds[i + 1];
                    const auto &a = engine.data.bins[bA];
                    const auto &b = engine.data.bins[bB];
                    DrawLineEx({a.position.x, a.position.y}, {b.position.x, b.position.y}, 2.0f, ColorAlpha(tourColor, 0.5f));
                }
            }
        }
    }

    void Renderer::renderOverlay_LoadSelect()
    {
        // USER SPEC: "Knapsack stage outlines the chosen subset in a solid ring vs. greyed-out unchosen bins"
        const AlgorithmStep *step = engine.activeTrace.getCurrentStep();
        const std::vector<int> &chosen = (step && step->type == TraceType::LOAD_SELECT_KNAPSACK)
                                             ? step->knapsackSelectedBins
                                             : engine.lastKnapsackResult.selectedBinIds;

        for (const auto &bin : engine.data.bins)
        {
            bool isSelected = (std::find(chosen.begin(), chosen.end(), bin.id) != chosen.end());

            if (isSelected)
            {
                // Solid green highlight ring
                DrawCircleLines(static_cast<int>(bin.position.x), static_cast<int>(bin.position.y),
                                22.0f, UIComponents::COLOR_GREEN);
                DrawCircleLines(static_cast<int>(bin.position.x), static_cast<int>(bin.position.y),
                                24.0f, UIComponents::COLOR_GREEN);

                // Waste weight badge
                char vBuf[32];
                std::snprintf(vBuf, sizeof(vBuf), "%.0fkg", bin.currentWasteKg);
                UIComponents::drawText(vBuf, bin.position.x - 16.0f, bin.position.y - 28.0f,
                                       9.0f, UIComponents::COLOR_GREEN);
            }
            else if (bin.currentWasteKg > 30.0)
            {
                // Greyed-out unchosen bin
                DrawCircle(static_cast<int>(bin.position.x), static_cast<int>(bin.position.y),
                           18.0f, {20, 24, 34, 160});
            }
        }
    }

    void Renderer::renderOverlay_LandfillBalance()
    {
        float time = static_cast<float>(GetTime());

        // USER SPEC: "Max-Flow stage swaps the map for a flow-arrow overlay (trucks → landfills, arrow thickness = volume, capped edges flash if binding)"
        for (const auto &v : engine.data.vehicles)
        {
            if (v.targetLandfillId < 0 || v.targetLandfillId >= static_cast<int>(engine.data.landfills.size()))
                continue;

            const auto &lf = engine.data.landfills[v.targetLandfillId];
            Vector2 truckPos = {v.currentPosition.x, v.currentPosition.y};
            Vector2 lfPos = {lf.position.x, lf.position.y};

            // Arrow thickness proportional to payload volume
            float thickness = 2.5f + static_cast<float>(v.currentLoadKg / 500.0) * 3.5f;
            Color flowColor = (v.targetLandfillId == 0) ? UIComponents::COLOR_CYAN : UIComponents::COLOR_PURPLE;

            DrawLineEx(truckPos, lfPos, thickness, ColorAlpha(flowColor, 0.45f));

            // Arrowhead
            Vector2 dir = {lfPos.x - truckPos.x, lfPos.y - truckPos.y};
            float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
            if (len > 1.0f)
            {
                dir.x /= len;
                dir.y /= len;
                Vector2 arrowTip = {lfPos.x - dir.x * 24.0f, lfPos.y - dir.y * 24.0f};
                Vector2 perp = {-dir.y * 7.0f, dir.x * 7.0f};
                DrawTriangle(
                    {arrowTip.x + dir.x * 12.0f, arrowTip.y + dir.y * 12.0f},
                    {arrowTip.x + perp.x, arrowTip.y + perp.y},
                    {arrowTip.x - perp.x, arrowTip.y - perp.y},
                    ColorAlpha(flowColor, 0.7f));
            }

            // Midpoint volume label
            Vector2 mid = {(truckPos.x + lfPos.x) * 0.5f, (truckPos.y + lfPos.y) * 0.5f};
            char loadBuf[32];
            std::snprintf(loadBuf, sizeof(loadBuf), "%.0fkg", v.currentLoadKg);
            UIComponents::drawText(loadBuf, mid.x - 14.0f, mid.y - 12.0f, 9.0f, flowColor);
        }

        // Capped edges flash if binding
        for (const auto &lf : engine.data.landfills)
        {
            if (lf.getIntakePercentage() >= 80.0)
            {
                float pulse = (std::sin(time * 8.0f) + 1.0f) * 0.5f;
                DrawCircleLines(static_cast<int>(lf.position.x), static_cast<int>(lf.position.y),
                                32.0f + pulse * 8.0f, ColorAlpha(UIComponents::COLOR_RED, 0.9f - pulse * 0.5f));
                UIComponents::drawText("BINDING CAPACITY!", lf.position.x - 50.0f, lf.position.y + 36.0f, 10.0f, UIComponents::COLOR_RED);
            }
        }
    }

    // ─────────────────────────────────────────────────────────────
    //  INLINE MAP EDITORS (Road weights & Bin creation)
    // ─────────────────────────────────────────────────────────────

    void Renderer::renderInlineRoadEditor()
    {
        if (!roadEditor.active || roadEditor.edgeId < 0 || roadEditor.edgeId >= engine.data.graph.getEdgeCount())
            return;

        auto &edge = engine.data.graph.getEdge(roadEditor.edgeId);
        float w = 240.0f;
        float h = 180.0f;
        float x = roadEditor.screenPos.x;
        float y = roadEditor.screenPos.y;

        if (x + w > GetScreenWidth() - 10)
            x = GetScreenWidth() - w - 10;
        if (y + h > GetScreenHeight() - 80)
            y = GetScreenHeight() - h - 80;

        Rectangle rPanel = {x, y, w, h};
        DrawRectangleRounded(rPanel, 0.08f, 4, {18, 24, 36, 250});
        DrawRectangleRoundedLinesEx(rPanel, 0.08f, 4, 1.5f, UIComponents::COLOR_CYAN);

        UIComponents::drawText(edge.roadName.c_str(), x + 10.0f, y + 8.0f, 13.0f, UIComponents::COLOR_CYAN);

        // Close 'X' button
        Rectangle rClose = {x + w - 24.0f, y + 6.0f, 18.0f, 18.0f};
        if (UIComponents::drawButton(rClose, "x"))
        {
            roadEditor.active = false;
            return;
        }

        float curY = y + 30.0f;
        // Speed adjustment
        char spdBuf[32];
        std::snprintf(spdBuf, sizeof(spdBuf), "Speed: %.0f km/h", roadEditor.tempSpeed);
        UIComponents::drawText(spdBuf, x + 10.0f, curY, 11.0f, UIComponents::TEXT_PRIMARY);

        if (UIComponents::drawButton({x + 130.0f, curY - 2.0f, 26.0f, 20.0f}, "-"))
            roadEditor.tempSpeed = std::max(10.0f, roadEditor.tempSpeed - 5.0f);
        if (UIComponents::drawButton({x + 162.0f, curY - 2.0f, 26.0f, 20.0f}, "+"))
            roadEditor.tempSpeed = std::min(80.0f, roadEditor.tempSpeed + 5.0f);

        curY += 28.0f;
        // Congestion adjustment
        roadEditor.tempCongestion = UIComponents::drawSlider({x + 10.0f, curY, w - 20.0f, 40.0f},
                                                             "Congestion Factor", roadEditor.tempCongestion, 1.0f, 5.0f);
        curY += 48.0f;

        // Toggle Closed button
        if (UIComponents::drawButton({x + 10.0f, curY, (w - 26.0f) * 0.5f, 26.0f}, roadEditor.tempClosed ? "Reopen" : "Close Road", roadEditor.tempClosed))
        {
            roadEditor.tempClosed = !roadEditor.tempClosed;
        }

        // Apply button
        if (UIComponents::drawButton({x + 16.0f + (w - 26.0f) * 0.5f, curY, (w - 26.0f) * 0.5f, 26.0f}, "Apply", true))
        {
            engine.updateRoadEdge(roadEditor.edgeId, roadEditor.tempSpeed, roadEditor.tempCongestion, roadEditor.tempClosed);
            roadEditor.active = false;
        }
    }

    void Renderer::renderInlineBinEditor()
    {
        if (!binEditor.active)
            return;

        float w = 240.0f;
        float h = (binEditor.binId >= 0) ? 140.0f : 170.0f;
        float x = binEditor.screenPos.x;
        float y = binEditor.screenPos.y;

        if (x + w > GetScreenWidth() - 10)
            x = GetScreenWidth() - w - 10;
        if (y + h > GetScreenHeight() - 80)
            y = GetScreenHeight() - h - 80;

        Rectangle rPanel = {x, y, w, h};
        DrawRectangleRounded(rPanel, 0.08f, 4, {18, 24, 36, 250});
        DrawRectangleRoundedLinesEx(rPanel, 0.08f, 4, 1.5f, UIComponents::COLOR_GOLD);

        // Close button
        Rectangle rClose = {x + w - 24.0f, y + 6.0f, 18.0f, 18.0f};
        if (UIComponents::drawButton(rClose, "x"))
        {
            binEditor.active = false;
            return;
        }

        if (binEditor.binId >= 0)
        {
            // Existing Bin Actions
            const auto &bin = engine.data.bins[binEditor.binId];
            UIComponents::drawText(bin.name.c_str(), x + 10.0f, y + 8.0f, 13.0f, UIComponents::COLOR_GOLD);

            char statBuf[64];
            std::snprintf(statBuf, sizeof(statBuf), "Waste: %.0f / %.0f kg (%.0f%%)",
                          bin.currentWasteKg, bin.capacityKg, bin.getFillRatio() * 100.0);
            UIComponents::drawText(statBuf, x + 10.0f, y + 32.0f, 11.0f, UIComponents::TEXT_MUTED);

            if (UIComponents::drawButton({x + 10.0f, y + 58.0f, w - 20.0f, 28.0f}, "Trigger 750kg Overflow", true))
            {
                engine.triggerBinOverflow(binEditor.binId, 750.0);
                binEditor.active = false;
            }

            if (UIComponents::drawButton({x + 10.0f, y + 94.0f, w - 20.0f, 28.0f}, "Decommission Bin"))
            {
                engine.removeBin(binEditor.binId);
                binEditor.active = false;
            }
        }
        else
        {
            // Create New Bin on Empty Space
            UIComponents::drawText("Place New Community Bin / STS", x + 10.0f, y + 8.0f, 12.0f, UIComponents::COLOR_CYAN);

            char capBuf[32];
            std::snprintf(capBuf, sizeof(capBuf), "Capacity: %.0f kg", binEditor.capacityKg);
            UIComponents::drawText(capBuf, x + 10.0f, y + 34.0f, 11.0f, UIComponents::TEXT_PRIMARY);

            if (UIComponents::drawButton({x + 140.0f, y + 32.0f, 40.0f, 20.0f}, "1.5t"))
                binEditor.capacityKg = 1500.0f;
            if (UIComponents::drawButton({x + 185.0f, y + 32.0f, 40.0f, 20.0f}, "2.5t"))
                binEditor.capacityKg = 2500.0f;

            char wBuf[32];
            std::snprintf(wBuf, sizeof(wBuf), "Initial Load: %.0f kg", binEditor.initialWasteKg);
            UIComponents::drawText(wBuf, x + 10.0f, y + 64.0f, 11.0f, UIComponents::TEXT_PRIMARY);

            if (UIComponents::drawButton({x + 140.0f, y + 62.0f, 40.0f, 20.0f}, "500"))
                binEditor.initialWasteKg = 500.0f;
            if (UIComponents::drawButton({x + 185.0f, y + 62.0f, 40.0f, 20.0f}, "900"))
                binEditor.initialWasteKg = 900.0f;

            if (UIComponents::drawButton({x + 10.0f, y + 100.0f, w - 20.0f, 30.0f}, "Create Bin", true))
            {
                std::string bName = "STS-Point-" + std::to_string(engine.data.bins.size() + 1);
                engine.addNewBin(bName, binEditor.worldPos, binEditor.capacityKg, binEditor.initialWasteKg);
                binEditor.active = false;
            }
        }
    }

    // ─────────────────────────────────────────────────────────────
    //  LEFT RAIL POPOVERS (Layers, Settings, Alerts)
    // ─────────────────────────────────────────────────────────────

    void Renderer::renderRailPopovers()
    {
        if (activePopover == 0)
            return;

        float popW = 230.0f;
        float popH = 260.0f;
        float popX = iconRailRect.x + iconRailRect.width + 4.0f;
        float popY = iconRailRect.y + 10.0f;

        Rectangle rPop = {popX, popY, popW, popH};
        DrawRectangleRounded(rPop, 0.08f, 4, {20, 26, 38, 250});
        DrawRectangleRoundedLinesEx(rPop, 0.08f, 4, 1.5f, UIComponents::COLOR_CYAN);

        // Close button
        if (UIComponents::drawButton({popX + popW - 24.0f, popY + 6.0f, 18.0f, 18.0f}, "x"))
        {
            activePopover = 0;
            return;
        }

        if (activePopover == 1) // LAYERS POPOVER
        {
            UIComponents::drawText("Map Layer Visibility", popX + 12.0f, popY + 10.0f, 13.0f, UIComponents::COLOR_CYAN);
            float ly = popY + 36.0f;

            if (UIComponents::drawButton({popX + 12.0f, ly, popW - 24.0f, 26.0f}, layerFlags[0] ? "[x] Road Network" : "[ ] Road Network", layerFlags[0]))
                layerFlags[0] = !layerFlags[0];
            ly += 32.0f;

            if (UIComponents::drawButton({popX + 12.0f, ly, popW - 24.0f, 26.0f}, layerFlags[1] ? "[x] Waste Bins" : "[ ] Waste Bins", layerFlags[1]))
                layerFlags[1] = !layerFlags[1];
            ly += 32.0f;

            if (UIComponents::drawButton({popX + 12.0f, ly, popW - 24.0f, 26.0f}, layerFlags[2] ? "[x] Fleet Vehicles" : "[ ] Fleet Vehicles", layerFlags[2]))
                layerFlags[2] = !layerFlags[2];
            ly += 32.0f;

            if (UIComponents::drawButton({popX + 12.0f, ly, popW - 24.0f, 26.0f}, layerFlags[3] ? "[x] Waterways / Districts" : "[ ] Waterways / Districts", layerFlags[3]))
                layerFlags[3] = !layerFlags[3];
            ly += 32.0f;

            if (UIComponents::drawButton({popX + 12.0f, ly, popW - 24.0f, 26.0f}, showNodeIds ? "[x] Show Node IDs" : "[ ] Show Node IDs", showNodeIds))
                showNodeIds = !showNodeIds;
        }
        else if (activePopover == 2) // SETTINGS POPOVER
        {
            UIComponents::drawText("Simulation Settings", popX + 12.0f, popY + 10.0f, 13.0f, UIComponents::COLOR_CYAN);
            float sy = popY + 36.0f;

            engine.wasteGenerationRateMultiplier = UIComponents::drawSlider({popX + 12.0f, sy, popW - 24.0f, 40.0f},
                                                                            "Waste Growth Rate", static_cast<float>(engine.wasteGenerationRateMultiplier), 0.5f, 3.0f);
            sy += 50.0f;

            engine.simSpeedMultiplier = UIComponents::drawSlider({popX + 12.0f, sy, popW - 24.0f, 40.0f},
                                                                 "Sim Speed Multiplier", engine.simSpeedMultiplier, 0.5f, 20.0f);
            sy += 50.0f;

            if (UIComponents::drawButton({popX + 12.0f, sy, popW - 24.0f, 28.0f}, "Reset All Congestion"))
            {
                engine.data.graph.resetAllCongestion();
                engine.activeDisruptions.clear();
            }
        }
        else if (activePopover == 3) // ALERTS POPOVER
        {
            UIComponents::drawText("Active Disruption Alerts", popX + 12.0f, popY + 10.0f, 13.0f, UIComponents::COLOR_GOLD);
            float ay = popY + 34.0f;

            int count = 0;
            for (auto &d : engine.activeDisruptions)
            {
                if (!d.isActive)
                    continue;
                char dBuf[64];
                std::snprintf(dBuf, sizeof(dBuf), "* %s", d.description.c_str());
                UIComponents::drawText(dBuf, popX + 12.0f, ay, 11.0f, d.type == DisruptionType::ROAD_CLOSURE ? UIComponents::COLOR_RED : UIComponents::COLOR_GOLD);
                ay += 20.0f;
                if (++count >= 5)
                    break;
            }

            if (count == 0)
            {
                UIComponents::drawText("No active disruptions.", popX + 12.0f, ay, 11.0f, UIComponents::TEXT_MUTED);
                ay += 24.0f;
            }

            if (UIComponents::drawButton({popX + 12.0f, popY + popH - 38.0f, popW - 24.0f, 28.0f}, "Clear All Disruptions"))
            {
                engine.data.graph.resetAllCongestion();
                engine.activeDisruptions.clear();
            }
        }
    }

    // ─────────────────────────────────────────────────────────────
    //  RIGHT PANEL
    // ─────────────────────────────────────────────────────────────

    void Renderer::renderRightPanel()
    {
        DrawRectangleRec(rightPanelRect, UIComponents::PANEL_BG);
        DrawLine(static_cast<int>(rightPanelRect.x), static_cast<int>(rightPanelRect.y),
                 static_cast<int>(rightPanelRect.x), static_cast<int>(rightPanelRect.y + rightPanelRect.height),
                 UIComponents::PANEL_BORDER);

        float tabY = rightPanelRect.y + 8.0f;
        float tabH = 28.0f;
        float tabW = (rightPanelRect.width - 32.0f) / 3.0f;
        float tabX = rightPanelRect.x + 8.0f;

        if (UIComponents::drawTabButton({tabX, tabY, tabW, tabH}, "Live feed",
                                        activeTab == RightPanelTab::LIVE_FEED))
            activeTab = RightPanelTab::LIVE_FEED;

        if (UIComponents::drawTabButton({tabX + tabW + 4.0f, tabY, tabW, tabH}, "Parameters",
                                        activeTab == RightPanelTab::PARAMETERS))
            activeTab = RightPanelTab::PARAMETERS;

        if (UIComponents::drawTabButton({tabX + (tabW + 4.0f) * 2.0f, tabY, tabW, tabH}, "Results",
                                        activeTab == RightPanelTab::RESULTS))
            activeTab = RightPanelTab::RESULTS;

        Rectangle contentRect = {
            rightPanelRect.x + 8.0f,
            tabY + tabH + 8.0f,
            rightPanelRect.width - 16.0f,
            rightPanelRect.height - tabH - 24.0f};

        DrawLine(static_cast<int>(rightPanelRect.x + 8), static_cast<int>(contentRect.y - 4),
                 static_cast<int>(rightPanelRect.x + rightPanelRect.width - 8), static_cast<int>(contentRect.y - 4),
                 UIComponents::PANEL_BORDER);

        switch (activeTab)
        {
        case RightPanelTab::LIVE_FEED:
            renderRightPanel_LiveFeed();
            break;
        case RightPanelTab::PARAMETERS:
            renderRightPanel_Parameters();
            break;
        case RightPanelTab::RESULTS:
            renderRightPanel_Results();
            break;
        }
    }

    void Renderer::renderRightPanel_LiveFeed()
    {
        float x = rightPanelRect.x + 12.0f;
        float y = rightPanelRect.y + 52.0f;
        float maxY = rightPanelRect.y + rightPanelRect.height - 14.0f;

        const char *stageNames[] = {"Road Network", "Routing (A*)", "Sequencing (Greedy)",
                                    "Load Select (Knapsack)", "Landfill Balance (Max-Flow)"};
        char filterBuf[64];
        std::snprintf(filterBuf, sizeof(filterBuf), "Active Stage: %s", stageNames[static_cast<int>(activeStage)]);
        UIComponents::drawText(filterBuf, x, y, 11.0f, UIComponents::COLOR_CYAN);
        y += 20.0f;

        DrawLine(static_cast<int>(x), static_cast<int>(y), static_cast<int>(rightPanelRect.x + rightPanelRect.width - 12),
                 static_cast<int>(y), UIComponents::PANEL_BORDER);
        y += 8.0f;

        // Display plain-language live feed entries synced to simulation
        int shown = 0;
        for (int i = static_cast<int>(engine.liveFeedLog.size()) - 1; i >= 0 && y < maxY; --i)
        {
            const auto &entry = engine.liveFeedLog[i];

            int hours = static_cast<int>(entry.timestamp / 3600.0) % 24;
            int mins = static_cast<int>(entry.timestamp / 60.0) % 60;
            char tsBuf[16];
            std::snprintf(tsBuf, sizeof(tsBuf), "%02d:%02d", hours, mins);
            UIComponents::drawText(tsBuf, x, y, 10.0f, UIComponents::COLOR_CYAN);

            float msgX = x + 40.0f;
            float msgW = rightPanelRect.width - 64.0f;
            const std::string &msg = entry.message;
            int maxChars = static_cast<int>(msgW / 6.5f);
            std::string displayMsg = msg.length() > static_cast<size_t>(maxChars) ? msg.substr(0, maxChars - 3) + "..." : msg;
            UIComponents::drawText(displayMsg.c_str(), msgX, y, 11.0f, UIComponents::TEXT_PRIMARY);

            y += 22.0f;
            shown++;
            if (shown >= 18)
                break;
        }

        if (shown == 0)
        {
            UIComponents::drawText("Awaiting live fleet telemetry...", x, y, 11.0f, UIComponents::TEXT_MUTED);
        }
    }

    void Renderer::renderRightPanel_Parameters()
    {
        float x = rightPanelRect.x + 12.0f;
        float y = rightPanelRect.y + 52.0f;
        float sliderW = rightPanelRect.width - 24.0f;
        float sliderH = 46.0f;

        // 1. Truck Default Capacity Slider
        float newTruckCap = UIComponents::drawSlider({x, y, sliderW, sliderH},
                                                     "Truck Default Capacity (kg)", static_cast<float>(engine.truckDefaultCapacityKg), 2000.0f, 8000.0f);
        engine.truckDefaultCapacityKg = newTruckCap;
        y += sliderH + 10.0f;

        // 2. Aminbazar Max Intake Slider
        float newAminCap = UIComponents::drawSlider({x, y, sliderW, sliderH},
                                                    "Aminbazar Daily Intake (kg)", static_cast<float>(engine.data.landfills[0].dailyCapacityKg), 10000.0f, 50000.0f);
        engine.data.landfills[0].dailyCapacityKg = newAminCap;
        y += sliderH + 10.0f;

        // 3. Matuail Max Intake Slider
        float newMatCap = UIComponents::drawSlider({x, y, sliderW, sliderH},
                                                   "Matuail Daily Intake (kg)", static_cast<float>(engine.data.landfills[1].dailyCapacityKg), 10000.0f, 50000.0f);
        engine.data.landfills[1].dailyCapacityKg = newMatCap;
        y += sliderH + 10.0f;

        // 4. Waste Generation Multiplier Slider
        float newGenRate = UIComponents::drawSlider({x, y, sliderW, sliderH},
                                                    "Waste Accumulation Rate", static_cast<float>(engine.wasteGenerationRateMultiplier), 0.5f, 3.0f);
        engine.wasteGenerationRateMultiplier = newGenRate;
        y += sliderH + 14.0f;

        // Map Click Hint
        UIComponents::drawText("Direct Map Actions:", x, y, 12.0f, UIComponents::COLOR_GOLD);
        y += 18.0f;
        UIComponents::drawText("* Click any road to edit base speed & congestion", x, y, 10.0f, UIComponents::TEXT_MUTED);
        y += 16.0f;
        UIComponents::drawText("* Click empty space to add new STS bin", x, y, 10.0f, UIComponents::TEXT_MUTED);
    }

    void Renderer::renderRightPanel_Results()
    {
        float x = rightPanelRect.x + 10.0f;
        float y = rightPanelRect.y + 52.0f;
        float cardW = rightPanelRect.width - 20.0f;
        float cardH = 58.0f;

        // USER SPEC: "plain numbers: total distance, urgency points covered, landfill split, capacity slack per truck. No jargon, just the outcome."

        // 1. Total Distance
        char distBuf[32];
        std::snprintf(distBuf, sizeof(distBuf), "%.1f km", engine.metrics.totalFleetDistanceKm);
        UIComponents::drawMetricCard({x, y, cardW, cardH},
                                     "TOTAL FLEET DISTANCE", distBuf, "Kilometers Logged", UIComponents::COLOR_CYAN);
        y += cardH + 8.0f;

        // 2. Urgency Points Covered
        char urgBuf[32];
        std::snprintf(urgBuf, sizeof(urgBuf), "%.1f%%", engine.metrics.urgencyCoveragePercent);
        UIComponents::drawMetricCard({x, y, cardW, cardH},
                                     "URGENCY POINTS COVERED", urgBuf, "City Waste Mitigated", UIComponents::COLOR_GREEN);
        y += cardH + 8.0f;

        // 3. Landfill Split
        double totalIntake = engine.data.landfills[0].currentIntakeKg + engine.data.landfills[1].currentIntakeKg;
        double aminPct = totalIntake > 0 ? (engine.data.landfills[0].currentIntakeKg / totalIntake * 100.0) : 50.0;
        double matPct = 100.0 - aminPct;

        char splitBuf[64];
        std::snprintf(splitBuf, sizeof(splitBuf), "Amin: %.0f%% | Mat: %.0f%%", aminPct, matPct);
        UIComponents::drawMetricCard({x, y, cardW, cardH},
                                     "LANDFILL LOAD SPLIT", splitBuf,
                                     engine.metrics.landfillBalanceRatio >= 0.7 ? "Perfect Edmonds-Karp Balance" : "Re-balancing in progress",
                                     UIComponents::COLOR_PURPLE);
        y += cardH + 8.0f;

        // 4. Truck Capacity Slack
        UIComponents::drawText("Capacity Slack Per Truck:", x + 4.0f, y, 12.0f, UIComponents::TEXT_PRIMARY);
        y += 18.0f;

        for (const auto &v : engine.data.vehicles)
        {
            double slackKg = v.getRemainingCapacity();
            double slackPct = (slackKg / v.capacityKg) * 100.0;
            char truckBuf[64];
            std::snprintf(truckBuf, sizeof(truckBuf), "%s: %.0f kg free (%.0f%%)",
                          v.name.c_str(), slackKg, slackPct);
            UIComponents::drawText(truckBuf, x + 6.0f, y, 10.0f,
                                   slackPct < 15.0 ? UIComponents::COLOR_RED : UIComponents::TEXT_MUTED);
            y += 16.0f;
        }
    }

    // ─────────────────────────────────────────────────────────────
    //  TOOLTIPS
    // ─────────────────────────────────────────────────────────────

    void Renderer::renderTooltips()
    {
        Vector2 mousePos = GetMousePosition();

        if (hoveredBinId >= 0 && hoveredBinId < static_cast<int>(engine.data.bins.size()))
        {
            const auto &bin = engine.data.bins[hoveredBinId];
            std::vector<std::pair<std::string, std::string>> fields = {
                {"Zone:", corporationToString(bin.corporation)},
                {"Current Waste:", std::to_string(static_cast<int>(bin.currentWasteKg)) + " kg"},
                {"Capacity:", std::to_string(static_cast<int>(bin.capacityKg)) + " kg"},
                {"Fill Ratio:", std::to_string(static_cast<int>(bin.getFillRatio() * 100)) + " %"},
                {"Urgency Score:", std::to_string(bin.urgencyScore).substr(0, 4)},
                {"Status:", bin.isOverflowing() ? "OVERFLOWING!" : (bin.isAssigned ? "Truck En Route" : "Accumulating")}};
            UIComponents::drawTooltip({mousePos.x + 15, mousePos.y + 15}, bin.name, fields);
        }
        else if (hoveredVehicleId >= 0 && hoveredVehicleId < static_cast<int>(engine.data.vehicles.size()))
        {
            const auto &v = engine.data.vehicles[hoveredVehicleId];
            std::vector<std::pair<std::string, std::string>> fields = {
                {"Corporation:", corporationToString(v.corporation)},
                {"Mission State:", vehicleStateToString(v.state)},
                {"Payload:", std::to_string(static_cast<int>(v.currentLoadKg)) + " / " + std::to_string(static_cast<int>(v.capacityKg)) + " kg"},
                {"Utilization:", std::to_string(static_cast<int>(v.getLoadPercentage())) + " %"},
                {"Trips Finished:", std::to_string(v.completedTrips)},
                {"Total Hauled:", std::to_string(static_cast<int>(v.totalWasteCollectedKg)) + " kg"}};
            UIComponents::drawTooltip({mousePos.x + 15, mousePos.y + 15}, v.name, fields);
        }
        else if (hoveredEdgeId >= 0 && hoveredEdgeId < engine.data.graph.getEdgeCount())
        {
            const auto &edge = engine.data.graph.getEdge(hoveredEdgeId);
            std::vector<std::pair<std::string, std::string>> fields = {
                {"Length:", std::to_string(static_cast<int>(edge.lengthMeters)) + " meters"},
                {"Base Speed:", std::to_string(static_cast<int>(edge.baseSpeedKmh)) + " km/h"},
                {"Congestion:", edge.isClosed ? "CLOSED" : (std::to_string(edge.congestionFactor).substr(0, 3) + "x")},
                {"Est. Travel:", edge.isClosed ? "BLOCKED" : (std::to_string(static_cast<int>(edge.getTravelTimeSeconds(engine.getHourOfDay()))) + " sec")},
                {"Click Action:", "Click to edit weight directly"}};
            UIComponents::drawTooltip({mousePos.x + 15, mousePos.y + 15}, edge.roadName, fields);
        }
        else if (hoveredLandfillId >= 0 && hoveredLandfillId < static_cast<int>(engine.data.landfills.size()))
        {
            const auto &lf = engine.data.landfills[hoveredLandfillId];
            std::vector<std::pair<std::string, std::string>> fields = {
                {"Primary Corp:", corporationToString(lf.corporationServed)},
                {"Daily Intake:", std::to_string(static_cast<int>(lf.currentIntakeKg)) + " / " + std::to_string(static_cast<int>(lf.dailyCapacityKg)) + " kg"},
                {"Capacity Used:", std::to_string(static_cast<int>(lf.getIntakePercentage())) + " %"},
                {"Trucks Queued:", std::to_string(lf.queueCount)},
                {"Balancing:", "Max-Flow Edmonds-Karp"}};
            UIComponents::drawTooltip({mousePos.x + 15, mousePos.y + 15}, lf.name, fields);
        }
    }

} // namespace dhaka
