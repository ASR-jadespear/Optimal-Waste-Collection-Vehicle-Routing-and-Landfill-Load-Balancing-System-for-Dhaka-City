#include "ui/Renderer.hpp"
#include <raymath.h>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <algorithm>

namespace dhaka
{

    Renderer::Renderer(SimulationEngine &eng, int screenWidth, int screenHeight)
        : engine(eng), camera(screenWidth, screenHeight)
    {
        UIComponents::initFont();
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
        float rightPanelW = std::max(280.0f, w * 0.28f);

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
        camera.update(dt);
        handleInteractions();

        // Keyboard shortcuts
        if (IsKeyPressed(KEY_SPACE))
            engine.isPaused = !engine.isPaused;
        if (IsKeyPressed(KEY_ONE))
            activeStage = PipelineStage::ROAD_NETWORK;
        if (IsKeyPressed(KEY_TWO))
            activeStage = PipelineStage::ROUTING;
        if (IsKeyPressed(KEY_THREE))
            activeStage = PipelineStage::SEQUENCING;
        if (IsKeyPressed(KEY_FOUR))
            activeStage = PipelineStage::LOAD_SELECT;
        if (IsKeyPressed(KEY_FIVE))
            activeStage = PipelineStage::LANDFILL_BALANCE;
        if (IsKeyPressed(KEY_R))
            engine.triggerRandomDhakaDisruption();

        // Speed controls
        if (IsKeyPressed(KEY_LEFT_BRACKET))
            engine.simSpeedMultiplier = std::max(0.5f, engine.simSpeedMultiplier * 0.5f);
        if (IsKeyPressed(KEY_RIGHT_BRACKET))
            engine.simSpeedMultiplier = std::min(20.0f, engine.simSpeedMultiplier * 2.0f);

        // Update timeline position based on sim time
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

        // Only interact with world if mouse is over the map area
        bool overMap = CheckCollisionPointRec(mouseScreen, mapRect);

        hoveredBinId = -1;
        hoveredEdgeId = -1;
        hoveredVehicleId = -1;
        hoveredLandfillId = -1;

        if (!overMap)
            return;

        // 1. Check bin hover & click
        for (const auto &bin : engine.data.bins)
        {
            float dist = mouseWorld.distanceTo(bin.position);
            float radius = 8.0f + static_cast<float>(bin.getFillRatio() * 12.0f);
            if (dist <= radius + 5.0f)
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
                        engine.triggerBinOverflow(bin.id, 750.0);
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
                if (dist <= 26.0f)
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
                        // Cycle: Free -> Congested -> Closed -> Free
                        const auto &edge = engine.data.graph.getEdge(edgeId);
                        if (!edge.isClosed && edge.congestionFactor <= 1.2)
                            engine.triggerRoadCongestion(edgeId, 4.5);
                        else if (!edge.isClosed && edge.congestionFactor > 1.2)
                            engine.triggerRoadClosure(edgeId);
                        else
                            engine.clearRoadDisruption(edgeId);
                    }
                }
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
                                                       engine.simTimeSeconds, engine.simSpeedMultiplier, isLiveMode);
        if (pauseClicked)
            engine.isPaused = !engine.isPaused;

        // 2. Pipeline Stepper
        int clickedStage = UIComponents::drawPipelineStepper(stepperRect, static_cast<int>(activeStage));
        if (clickedStage >= 0 && clickedStage <= 4)
            activeStage = static_cast<PipelineStage>(clickedStage);

        // 3. Map Region (camera-transformed)
        BeginScissorMode(static_cast<int>(mapRect.x), static_cast<int>(mapRect.y),
                         static_cast<int>(mapRect.width), static_cast<int>(mapRect.height));
        BeginMode2D(camera.camera);
        renderBaseMap();
        renderStageOverlay();
        EndMode2D();
        EndScissorMode();

        // 4. Icon Rail
        UIComponents::drawIconRail(iconRailRect, layerFlags);

        // 5. Right Panel
        renderRightPanel();

        // 6. Bottom Bar
        float newTimelinePos = UIComponents::drawTimelineScrubber(bottomBarRect, timelinePos, engine.isPaused);
        if (std::abs(newTimelinePos - timelinePos) > 0.001f)
        {
            timelinePos = newTimelinePos;
        }

        // Disruption buttons in bottom bar (right side)
        float dbX = bottomBarRect.x + bottomBarRect.width - 500.0f;
        float dbY = bottomBarRect.y + 14.0f;
        float dbW = 150.0f;
        float dbH = 42.0f;

        if (UIComponents::drawDisruptionButton({dbX, dbY, dbW, dbH}, "/\\", "Close road"))
        {
            disruptionMode = (disruptionMode == 1) ? 0 : 1;
        }
        if (UIComponents::drawDisruptionButton({dbX + dbW + 8.0f, dbY, dbW, dbH}, "!!", "Spike congestion"))
        {
            disruptionMode = (disruptionMode == 2) ? 0 : 2;
        }
        if (UIComponents::drawDisruptionButton({dbX + (dbW + 8.0f) * 2.0f, dbY, dbW, dbH}, "+", "New overflow"))
        {
            disruptionMode = (disruptionMode == 3) ? 0 : 3;
        }

        // Disruption mode indicator
        if (disruptionMode > 0)
        {
            const char *modeText = disruptionMode == 1 ? "Click a road to CLOSE it" : (disruptionMode == 2 ? "Click a road to CONGEST it" : "Click a bin to trigger OVERFLOW");
            int tw = UIComponents::measureText(modeText, 14.0f);
            DrawRectangle(static_cast<int>(mapRect.x + mapRect.width * 0.5f - tw * 0.5f - 12),
                          static_cast<int>(mapRect.y + 10), tw + 24, 28, {239, 68, 68, 200});
            UIComponents::drawText(modeText,
                                   mapRect.x + mapRect.width * 0.5f - tw * 0.5f,
                                   mapRect.y + 17.0f, 14.0f, WHITE);
        }

        // 7. Tooltips (on top of everything)
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
        float time = static_cast<float>(GetTime());

        for (const auto &edge : engine.data.graph.edges)
        {
            if (edge.fromNode > edge.toNode)
                continue;

            const auto &nA = engine.data.graph.getNode(edge.fromNode);
            const auto &nB = engine.data.graph.getNode(edge.toNode);

            Vector2 pA = {nA.position.x, nA.position.y};
            Vector2 pB = {nB.position.x, nB.position.y};

            Color roadColor;
            float roadThickness = 3.5f;

            if (edge.isClosed)
            {
                float pulse = (std::sin(time * 6.0f) + 1.0f) * 0.5f;
                roadColor = ColorAlpha(UIComponents::COLOR_RED, 0.4f + pulse * 0.5f);
                roadThickness = 4.0f;
            }
            else if (edge.congestionFactor >= 3.0)
            {
                roadColor = UIComponents::COLOR_RED;
                roadThickness = 4.5f;
            }
            else if (edge.congestionFactor > 1.4)
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

            if (edge.isClosed)
            {
                Vector2 mid = {(pA.x + pB.x) * 0.5f, (pA.y + pB.y) * 0.5f};
                DrawCircle(static_cast<int>(mid.x), static_cast<int>(mid.y), 7.0f, UIComponents::COLOR_RED);
                DrawLine(static_cast<int>(mid.x - 4), static_cast<int>(mid.y - 4),
                         static_cast<int>(mid.x + 4), static_cast<int>(mid.y + 4), WHITE);
                DrawLine(static_cast<int>(mid.x - 4), static_cast<int>(mid.y + 4),
                         static_cast<int>(mid.x + 4), static_cast<int>(mid.y - 4), WHITE);
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
        for (const auto &lf : engine.data.landfills)
        {
            Vector2 pos = {lf.position.x, lf.position.y};

            DrawRectanglePro({pos.x, pos.y, 34.0f, 34.0f}, {17.0f, 17.0f}, 45.0f,
                             lf.id == 0 ? UIComponents::COLOR_CYAN : UIComponents::COLOR_PURPLE);
            DrawRectangleLinesEx({pos.x - 19.0f, pos.y - 19.0f, 38.0f, 38.0f}, 2.0f, WHITE);

            UIComponents::drawText(lf.shortName.c_str(), pos.x - 28.0f, pos.y - 34.0f,
                                   12.0f, UIComponents::TEXT_PRIMARY);

            float p = static_cast<float>(lf.getIntakePercentage());
            Rectangle barRec = {pos.x - 26.0f, pos.y + 22.0f, 52.0f, 7.0f};
            UIComponents::drawProgressBar(barRec, p, p > 80.0f ? UIComponents::COLOR_RED : UIComponents::COLOR_GREEN, nullptr);

            if (lf.queueCount > 0)
            {
                DrawCircle(static_cast<int>(pos.x + 18.0f), static_cast<int>(pos.y - 16.0f), 8.0f, UIComponents::COLOR_GOLD);
                char qBuf[8];
                std::snprintf(qBuf, sizeof(qBuf), "%d", lf.queueCount);
                UIComponents::drawText(qBuf, pos.x + 15.0f, pos.y - 21.0f, 10.0f, BLACK);
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

            Rectangle truckRec = {pos.x, pos.y, 22.0f, 13.0f};
            Vector2 origin = {11.0f, 6.5f};
            DrawRectanglePro(truckRec, origin, v.headingAngle, truckColor);
            DrawRectangleLinesEx({pos.x - 11.0f, pos.y - 7.0f, 22.0f, 14.0f}, 1.0f, WHITE);

            // Cargo load ring (capacity-fill indicator)
            float loadPct = static_cast<float>(v.getLoadPercentage());
            float ringRadius = 14.0f;
            float ringAngle = loadPct / 100.0f * 360.0f;
            Color ringColor = loadPct > 85.0f ? UIComponents::COLOR_RED : UIComponents::COLOR_GREEN;

            // Draw ring as arc approximation
            DrawCircleLines(static_cast<int>(pos.x), static_cast<int>(pos.y), ringRadius, {30, 40, 55, 100});
            if (ringAngle > 0.0f)
            {
                DrawRing({pos.x, pos.y}, ringRadius - 2.0f, ringRadius, 0.0f, ringAngle, 24, ringColor);
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
        // Show edge weights (travel time) and node IDs on the map
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
                std::snprintf(timeBuf, sizeof(timeBuf), "%.0fs", edge.getTravelTimeSeconds());
            }
            UIComponents::drawText(timeBuf, mid.x - 12.0f, mid.y - 14.0f, 9.0f,
                                   edge.isClosed ? UIComponents::COLOR_RED : UIComponents::TEXT_MUTED);

            // Congestion factor badge
            if (!edge.isClosed && edge.congestionFactor > 1.2)
            {
                char congBuf[16];
                std::snprintf(congBuf, sizeof(congBuf), "%.1fx", edge.congestionFactor);
                UIComponents::drawText(congBuf, mid.x - 8.0f, mid.y + 2.0f, 8.0f,
                                       edge.congestionFactor >= 3.0 ? UIComponents::COLOR_RED : UIComponents::COLOR_GOLD);
            }
        }

        // Node ID labels
        if (camera.camera.zoom >= 0.6f)
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
        // Highlight active A* paths with thicker, brighter lines
        float time = static_cast<float>(GetTime());

        for (const auto &v : engine.data.vehicles)
        {
            if (v.pathNodeIds.size() < 2)
                continue;

            bool isActive = (v.state == VehicleState::EN_ROUTE_TO_BIN ||
                             v.state == VehicleState::EN_ROUTE_TO_LANDFILL);
            if (!isActive)
                continue;

            Color pathColor = (v.corporation == Corporation::DNCC)
                                  ? UIComponents::COLOR_CYAN
                                  : UIComponents::COLOR_GOLD;

            // Draw explored frontier glow effect
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

                // Animated dash effect
                float segProgress = static_cast<float>(i - v.currentPathSegmentIndex) /
                                    static_cast<float>(std::max(1, static_cast<int>(v.pathNodeIds.size()) - v.currentPathSegmentIndex - 1));
                float alpha = 0.8f - segProgress * 0.5f;
                float pulse = (std::sin(time * 4.0f + segProgress * 6.28f) + 1.0f) * 0.15f;

                DrawLineEx(p1, p2, 5.0f, ColorAlpha(pathColor, alpha + pulse));
            }

            // Draw path destination marker
            if (!v.pathNodeIds.empty())
            {
                int destNode = v.pathNodeIds.back();
                const auto &dest = engine.data.graph.getNode(destNode);
                float pulseR = (std::sin(time * 3.0f) + 1.0f) * 3.0f + 8.0f;
                DrawCircleLines(static_cast<int>(dest.position.x), static_cast<int>(dest.position.y),
                                pulseR, pathColor);
            }

            // A* label at vehicle
            UIComponents::drawText("A*", v.currentPosition.x + 14.0f,
                                   v.currentPosition.y - 22.0f, 10.0f, pathColor);
        }
    }

    void Renderer::renderOverlay_Sequencing()
    {
        // Show numbered visit order for each vehicle's assigned bins
        for (const auto &v : engine.data.vehicles)
        {
            if (v.assignedBinIds.empty())
                continue;

            Color tourColor = (v.corporation == Corporation::DNCC)
                                  ? UIComponents::COLOR_CYAN
                                  : UIComponents::COLOR_GOLD;

            // Draw numbered badges on bins
            for (size_t i = 0; i < v.assignedBinIds.size(); ++i)
            {
                int binId = v.assignedBinIds[i];
                if (binId < 0 || binId >= static_cast<int>(engine.data.bins.size()))
                    continue;

                const auto &bin = engine.data.bins[binId];

                // Visit order number badge
                char numBuf[8];
                std::snprintf(numBuf, sizeof(numBuf), "%d", static_cast<int>(i + 1));

                float badgeX = bin.position.x + 10.0f;
                float badgeY = bin.position.y - 16.0f;

                DrawCircle(static_cast<int>(badgeX), static_cast<int>(badgeY), 10.0f, tourColor);
                int tw = UIComponents::measureText(numBuf, 10.0f);
                UIComponents::drawText(numBuf, badgeX - tw * 0.5f, badgeY - 5.0f, 10.0f, WHITE);

                // Greyed out if already visited
                if (static_cast<int>(i) < v.currentTargetBinIndex)
                {
                    DrawCircle(static_cast<int>(bin.position.x), static_cast<int>(bin.position.y),
                               10.0f, {20, 24, 34, 160}); // semi-transparent overlay
                }
            }

            // Draw greedy tour connecting lines between consecutive bins
            for (size_t i = 0; i + 1 < v.assignedBinIds.size(); ++i)
            {
                int binA = v.assignedBinIds[i];
                int binB = v.assignedBinIds[i + 1];
                if (binA < 0 || binA >= static_cast<int>(engine.data.bins.size()))
                    continue;
                if (binB < 0 || binB >= static_cast<int>(engine.data.bins.size()))
                    continue;

                const auto &a = engine.data.bins[binA];
                const auto &b = engine.data.bins[binB];
                DrawLineEx({a.position.x, a.position.y}, {b.position.x, b.position.y},
                           2.0f, ColorAlpha(tourColor, 0.4f));
            }
        }
    }

    void Renderer::renderOverlay_LoadSelect()
    {
        // Show knapsack selection: selected bins outlined green, rejected greyed
        const auto &result = engine.lastKnapsackResult;

        for (const auto &bin : engine.data.bins)
        {
            bool isSelected = false;
            for (int id : result.selectedBinIds)
            {
                if (id == bin.id)
                {
                    isSelected = true;
                    break;
                }
            }

            if (isSelected)
            {
                // Green selection highlight
                DrawCircleLines(static_cast<int>(bin.position.x), static_cast<int>(bin.position.y),
                                22.0f, UIComponents::COLOR_GREEN);
                DrawCircleLines(static_cast<int>(bin.position.x), static_cast<int>(bin.position.y),
                                24.0f, UIComponents::COLOR_GREEN);

                // Value badge
                char vBuf[32];
                std::snprintf(vBuf, sizeof(vBuf), "%.0fkg", bin.currentWasteKg);
                UIComponents::drawText(vBuf, bin.position.x - 16.0f, bin.position.y - 28.0f,
                                       9.0f, UIComponents::COLOR_GREEN);
            }
            else if (bin.currentWasteKg > 30.0)
            {
                // Greyed out rejected candidate
                DrawCircle(static_cast<int>(bin.position.x), static_cast<int>(bin.position.y),
                           18.0f, {20, 24, 34, 140});
            }
        }
    }

    void Renderer::renderOverlay_LandfillBalance()
    {
        // Show flow arrows from trucks to landfills
        for (const auto &v : engine.data.vehicles)
        {
            if (v.targetLandfillId < 0 || v.targetLandfillId >= static_cast<int>(engine.data.landfills.size()))
                continue;
            if (v.state != VehicleState::EN_ROUTE_TO_LANDFILL && v.state != VehicleState::UNLOADING_AT_LANDFILL)
                continue;

            const auto &lf = engine.data.landfills[v.targetLandfillId];

            Vector2 truckPos = {v.currentPosition.x, v.currentPosition.y};
            Vector2 lfPos = {lf.position.x, lf.position.y};

            // Flow arrow thickness proportional to load
            float thickness = 2.0f + static_cast<float>(v.currentLoadKg / 500.0) * 3.0f;
            Color flowColor = (v.targetLandfillId == 0) ? UIComponents::COLOR_CYAN : UIComponents::COLOR_PURPLE;

            DrawLineEx(truckPos, lfPos, thickness, ColorAlpha(flowColor, 0.35f));

            // Arrowhead
            Vector2 dir = {lfPos.x - truckPos.x, lfPos.y - truckPos.y};
            float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
            if (len > 1.0f)
            {
                dir.x /= len;
                dir.y /= len;
                Vector2 arrowTip = {lfPos.x - dir.x * 22.0f, lfPos.y - dir.y * 22.0f};
                Vector2 perp = {-dir.y * 6.0f, dir.x * 6.0f};
                DrawTriangle(
                    {arrowTip.x + dir.x * 12.0f, arrowTip.y + dir.y * 12.0f},
                    {arrowTip.x + perp.x, arrowTip.y + perp.y},
                    {arrowTip.x - perp.x, arrowTip.y - perp.y},
                    ColorAlpha(flowColor, 0.6f));
            }

            // Load label at midpoint
            Vector2 mid = {(truckPos.x + lfPos.x) * 0.5f, (truckPos.y + lfPos.y) * 0.5f};
            char loadBuf[32];
            std::snprintf(loadBuf, sizeof(loadBuf), "%.0fkg", v.currentLoadKg);
            UIComponents::drawText(loadBuf, mid.x - 14.0f, mid.y - 12.0f, 9.0f, flowColor);
        }

        // Landfill capacity rings
        for (const auto &lf : engine.data.landfills)
        {
            Vector2 pos = {lf.position.x, lf.position.y};
            float intakePct = static_cast<float>(lf.getIntakePercentage());
            float ringAngle = intakePct / 100.0f * 360.0f;
            Color ringColor = intakePct > 80.0f   ? UIComponents::COLOR_RED
                              : intakePct > 50.0f ? UIComponents::COLOR_GOLD
                                                  : UIComponents::COLOR_GREEN;

            DrawRing(pos, 24.0f, 30.0f, 0.0f, 360.0f, 32, {30, 36, 50, 180});
            if (ringAngle > 0.0f)
            {
                DrawRing(pos, 24.0f, 30.0f, 0.0f, ringAngle, 32, ringColor);
            }

            char capBuf[32];
            std::snprintf(capBuf, sizeof(capBuf), "%.0f%%", intakePct);
            int tw = UIComponents::measureText(capBuf, 10.0f);
            UIComponents::drawText(capBuf, pos.x - tw * 0.5f, pos.y + 34.0f, 10.0f, ringColor);
        }
    }

    // ─────────────────────────────────────────────────────────────
    //  RIGHT PANEL
    // ─────────────────────────────────────────────────────────────

    void Renderer::renderRightPanel()
    {
        // Panel background
        DrawRectangleRec(rightPanelRect, UIComponents::PANEL_BG);
        DrawLine(static_cast<int>(rightPanelRect.x), static_cast<int>(rightPanelRect.y),
                 static_cast<int>(rightPanelRect.x), static_cast<int>(rightPanelRect.y + rightPanelRect.height),
                 UIComponents::PANEL_BORDER);

        // Tab buttons
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

        // Tab content area
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
        float x = rightPanelRect.x + 14.0f;
        float y = rightPanelRect.y + 52.0f;
        float maxY = rightPanelRect.y + rightPanelRect.height - 12.0f;

        // Stage filter label
        const char *stageNames[] = {"Road Network", "Routing (A*)", "Sequencing (Greedy)",
                                    "Load Select (Knapsack)", "Landfill Balance (Max-Flow)"};
        char filterBuf[64];
        std::snprintf(filterBuf, sizeof(filterBuf), "Stage: %s", stageNames[static_cast<int>(activeStage)]);
        UIComponents::drawText(filterBuf, x, y, 11.0f, UIComponents::COLOR_CYAN);
        y += 20.0f;
        DrawLine(static_cast<int>(x), static_cast<int>(y), static_cast<int>(rightPanelRect.x + rightPanelRect.width - 14),
                 static_cast<int>(y), UIComponents::PANEL_BORDER);
        y += 8.0f;

        // Show feed entries (most recent first), filtered by active stage
        int shown = 0;
        for (int i = static_cast<int>(engine.liveFeedLog.size()) - 1; i >= 0 && y < maxY; --i)
        {
            const auto &entry = engine.liveFeedLog[i];

            // Show all entries or filter by stage
            bool relevant = (entry.relevantStage == static_cast<int>(activeStage)) ||
                            (activeStage == PipelineStage::ROAD_NETWORK); // Road network shows all

            if (!relevant)
                continue;

            // Timestamp
            int hours = static_cast<int>(entry.timestamp / 3600.0) % 24;
            int mins = static_cast<int>(entry.timestamp / 60.0) % 60;
            char tsBuf[16];
            std::snprintf(tsBuf, sizeof(tsBuf), "%02d:%02d", hours, mins);
            UIComponents::drawText(tsBuf, x, y, 10.0f, UIComponents::TEXT_MUTED);

            // Message (wrap text)
            float msgX = x + 44.0f;
            float msgW = rightPanelRect.width - 70.0f;
            const std::string &msg = entry.message;
            int fontSize = 11;
            int maxChars = static_cast<int>(msgW / 6.5f);
            std::string displayMsg = msg.length() > static_cast<size_t>(maxChars) ? msg.substr(0, maxChars - 3) + "..." : msg;
            UIComponents::drawText(displayMsg.c_str(), msgX, y, static_cast<float>(fontSize), UIComponents::TEXT_PRIMARY);

            y += 22.0f;
            shown++;
            if (shown >= 20)
                break;
        }

        if (shown == 0)
        {
            UIComponents::drawText("No events yet for this stage.", x, y,
                                   11.0f, UIComponents::TEXT_MUTED);
        }
    }

    void Renderer::renderRightPanel_Parameters()
    {
        float x = rightPanelRect.x + 14.0f;
        float y = rightPanelRect.y + 52.0f;
        float sliderW = rightPanelRect.width - 28.0f;
        float sliderH = 48.0f;

        // Simulation Speed
        float newSpeed = UIComponents::drawSlider({x, y, sliderW, sliderH},
                                                  "Simulation Speed", engine.simSpeedMultiplier, 0.5f, 20.0f);
        engine.simSpeedMultiplier = newSpeed;
        y += sliderH + 12.0f;

        // Active Disruptions section
        UIComponents::drawText("Active Disruptions", x, y, 13.0f, UIComponents::TEXT_PRIMARY);
        y += 22.0f;
        DrawLine(static_cast<int>(x), static_cast<int>(y),
                 static_cast<int>(x + sliderW), static_cast<int>(y), UIComponents::PANEL_BORDER);
        y += 8.0f;

        int count = 0;
        for (auto it = engine.activeDisruptions.rbegin(); it != engine.activeDisruptions.rend(); ++it)
        {
            if (!it->isActive)
                continue;
            char dText[128];
            std::snprintf(dText, sizeof(dText), "* %s", it->description.c_str());
            UIComponents::drawText(dText, x, y, 11.0f,
                                   it->type == DisruptionType::ROAD_CLOSURE ? UIComponents::COLOR_RED : UIComponents::COLOR_GOLD);
            y += 18.0f;
            if (++count >= 6)
                break;
        }
        if (count == 0)
        {
            UIComponents::drawText("No active disruptions.", x, y,
                                   11.0f, UIComponents::TEXT_MUTED);
        }
    }

    void Renderer::renderRightPanel_Results()
    {
        float x = rightPanelRect.x + 8.0f;
        float y = rightPanelRect.y + 52.0f;
        float cardW = rightPanelRect.width - 16.0f;
        float cardH = 62.0f;

        // Metric Cards
        char valPending[32], valCollected[32], valOverflow[32], valFleet[32];
        std::snprintf(valPending, sizeof(valPending), "%.0f kg", engine.metrics.currentTotalPendingWasteKg);
        std::snprintf(valCollected, sizeof(valCollected), "%.0f kg", engine.metrics.totalWasteCollectedKg);
        std::snprintf(valOverflow, sizeof(valOverflow), "%d bins", engine.metrics.currentOverflowingBins);
        std::snprintf(valFleet, sizeof(valFleet), "%d / %d active", engine.metrics.activeTruckCount,
                      static_cast<int>(engine.data.vehicles.size()));

        UIComponents::drawMetricCard({x, y, cardW, cardH},
                                     "PENDING WASTE", valPending, "Bins Accumulating", UIComponents::COLOR_GOLD);
        y += cardH + 8.0f;

        UIComponents::drawMetricCard({x, y, cardW, cardH},
                                     "TOTAL COLLECTED", valCollected, "Shift Throughput", UIComponents::COLOR_GREEN);
        y += cardH + 8.0f;

        UIComponents::drawMetricCard({x, y, cardW, cardH},
                                     "OVERFLOW RISKS", valOverflow, "Priority Managed",
                                     engine.metrics.currentOverflowingBins > 0 ? UIComponents::COLOR_RED : UIComponents::COLOR_GREEN);
        y += cardH + 8.0f;

        UIComponents::drawMetricCard({x, y, cardW, cardH},
                                     "FLEET DISPATCH", valFleet, "Knapsack Scheduled", UIComponents::COLOR_CYAN);
        y += cardH + 16.0f;

        // Landfill Balance Section
        UIComponents::drawText("Landfill Balance (Max-Flow)", x + 6.0f, y, 13.0f, UIComponents::TEXT_PRIMARY);
        y += 22.0f;

        // Aminbazar bar
        float pAmin = engine.data.landfills[0].dailyCapacityKg > 0
                          ? static_cast<float>(engine.data.landfills[0].currentIntakeKg / engine.data.landfills[0].dailyCapacityKg * 100.0)
                          : 0.0f;
        char aminText[64];
        std::snprintf(aminText, sizeof(aminText), "Aminbazar: %.0f / %.0f kg",
                      engine.data.landfills[0].currentIntakeKg, engine.data.landfills[0].dailyCapacityKg);
        UIComponents::drawText(aminText, x + 6.0f, y, 11.0f, UIComponents::TEXT_PRIMARY);
        y += 16.0f;
        UIComponents::drawProgressBar({x + 6, y, cardW - 12, 14.0f}, pAmin,
                                      pAmin > 85.0f ? UIComponents::COLOR_RED : UIComponents::COLOR_CYAN, nullptr);
        y += 24.0f;

        // Matuail bar
        float pMat = engine.data.landfills[1].dailyCapacityKg > 0
                         ? static_cast<float>(engine.data.landfills[1].currentIntakeKg / engine.data.landfills[1].dailyCapacityKg * 100.0)
                         : 0.0f;
        char matText[64];
        std::snprintf(matText, sizeof(matText), "Matuail: %.0f / %.0f kg",
                      engine.data.landfills[1].currentIntakeKg, engine.data.landfills[1].dailyCapacityKg);
        UIComponents::drawText(matText, x + 6.0f, y, 11.0f, UIComponents::TEXT_PRIMARY);
        y += 16.0f;
        UIComponents::drawProgressBar({x + 6, y, cardW - 12, 14.0f}, pMat,
                                      pMat > 85.0f ? UIComponents::COLOR_RED : UIComponents::COLOR_PURPLE, nullptr);
        y += 24.0f;

        // Balance ratio
        char balBuf[64];
        std::snprintf(balBuf, sizeof(balBuf), "Edmonds-Karp Balance: %.1f%%",
                      engine.metrics.landfillBalanceRatio * 100.0);
        UIComponents::drawText(balBuf, x + 6.0f, y, 12.0f,
                               engine.metrics.landfillBalanceRatio >= 0.7 ? UIComponents::COLOR_GREEN : UIComponents::COLOR_GOLD);
        y += 18.0f;
        UIComponents::drawProgressBar({x + 6, y, cardW - 12, 12.0f},
                                      static_cast<float>(engine.metrics.landfillBalanceRatio * 100.0),
                                      engine.metrics.landfillBalanceRatio >= 0.7 ? UIComponents::COLOR_GREEN : UIComponents::COLOR_GOLD, nullptr);
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
                {"Est. Travel:", edge.isClosed ? "BLOCKED" : (std::to_string(static_cast<int>(edge.getTravelTimeSeconds())) + " sec")},
                {"Click Action:", edge.isClosed ? "Click to Reopen" : (edge.congestionFactor > 1.2 ? "Click to Close" : "Click to Congest")}};
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
