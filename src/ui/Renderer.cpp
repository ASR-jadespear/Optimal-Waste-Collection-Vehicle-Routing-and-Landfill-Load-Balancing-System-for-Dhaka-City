#include "ui/Renderer.hpp"
#include <raymath.h>
#include <cmath>
#include <cstdio>
#include <iostream>

namespace dhaka
{

    Renderer::Renderer(SimulationEngine &eng, int screenWidth, int screenHeight)
        : engine(eng), camera(screenWidth, screenHeight) {}

    void Renderer::update(float dt)
    {
        camera.update(dt);
        handleInteractions();

        // Hotkey Controls
        if (IsKeyPressed(KEY_SPACE))
            engine.isPaused = !engine.isPaused;
        if (IsKeyPressed(KEY_ONE))
            engine.simSpeedMultiplier = 1.0f;
        if (IsKeyPressed(KEY_TWO))
            engine.simSpeedMultiplier = 2.0f;
        if (IsKeyPressed(KEY_THREE))
            engine.simSpeedMultiplier = 5.0f;
        if (IsKeyPressed(KEY_FOUR))
            engine.simSpeedMultiplier = 10.0f;
        if (IsKeyPressed(KEY_R))
            engine.triggerRandomDhakaDisruption();
        if (IsKeyPressed(KEY_H))
            showHeatmap = !showHeatmap;
        if (IsKeyPressed(KEY_P))
            showPaths = !showPaths;
        if (IsKeyPressed(KEY_L))
            showLabels = !showLabels;
        if (IsKeyPressed(KEY_TAB))
            showSidebar = !showSidebar;
    }

    void Renderer::handleInteractions()
    {
        Vector2 mouseScreen = GetMousePosition();
        Vec2 mouseWorld = camera.screenToWorld(mouseScreen);

        // Ignore world clicks if mouse is over sidebar
        float sidebarX = GetScreenWidth() - 360.0f;
        bool overUI = (showSidebar && mouseScreen.x >= sidebarX) || (mouseScreen.y <= 95.0f);

        hoveredBinId = -1;
        hoveredEdgeId = -1;
        hoveredVehicleId = -1;
        hoveredLandfillId = -1;

        if (!overUI)
        {
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
                        engine.triggerBinOverflow(bin.id, 750.0);
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
                        // Cycle road state: Free -> Congested -> Closed -> Free
                        const auto &edge = engine.data.graph.getEdge(edgeId);
                        if (!edge.isClosed && edge.congestionFactor <= 1.2)
                        {
                            engine.triggerRoadCongestion(edgeId, 4.5);
                        }
                        else if (!edge.isClosed && edge.congestionFactor > 1.2)
                        {
                            engine.triggerRoadClosure(edgeId);
                        }
                        else
                        {
                            engine.clearRoadDisruption(edgeId);
                        }
                    }
                }
            }
        }
    }

    void Renderer::render()
    {
        BeginDrawing();
        ClearBackground(UIComponents::BG_DARK);

        // 1. World Scene inside 2D Camera
        BeginMode2D(camera.camera);
        renderWorld();
        EndMode2D();

        // 2. UI Dashboard and Overlays
        renderUI();

        EndDrawing();
    }

    void Renderer::renderWorld()
    {
        renderWaterways();
        renderRoads();
        if (showPaths)
            renderActivePaths();
        renderBins();
        renderLandfills();
        renderDepots();
        renderVehicles();
    }

    void Renderer::renderWaterways()
    {
        // Water bodies in soft river blue
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

        // District watermark labels
        for (const auto &[label, pos] : engine.data.districtLabels)
        {
            DrawText(label.c_str(), static_cast<int>(pos.x - 60.0f), static_cast<int>(pos.y),
                     16, {40, 50, 70, 160});
        }
    }

    void Renderer::renderRoads()
    {
        float time = static_cast<float>(GetTime());

        for (const auto &edge : engine.data.graph.edges)
        {
            // Only draw forward edge to avoid double drawing
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
                // Closed / Blocked road: flashing red-gray dashed pattern
                float pulse = (std::sin(time * 6.0f) + 1.0f) * 0.5f;
                roadColor = ColorAlpha(UIComponents::COLOR_RED, 0.4f + pulse * 0.5f);
                roadThickness = 4.0f;
            }
            else if (edge.congestionFactor >= 3.0)
            {
                roadColor = UIComponents::COLOR_RED; // Gridlock
                roadThickness = 4.5f;
            }
            else if (edge.congestionFactor > 1.4)
            {
                roadColor = UIComponents::COLOR_GOLD; // Heavy traffic
                roadThickness = 4.0f;
            }
            else
            {
                roadColor = Color{46, 75, 60, 220}; // Free flow muted green
                roadThickness = 2.8f;
            }

            // Highlight if hovered
            if (edge.id == hoveredEdgeId)
            {
                roadThickness += 2.0f;
                roadColor = UIComponents::COLOR_CYAN;
            }

            DrawLineEx(pA, pB, roadThickness, roadColor);

            // If road is closed, draw an 'X' barrier at midpoint
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

            Color pathColor = (v.corporation == Corporation::DNCC) ? ColorAlpha(UIComponents::COLOR_CYAN, 0.55f) : ColorAlpha(UIComponents::COLOR_GOLD, 0.55f);

            for (size_t i = v.currentPathSegmentIndex; i + 1 < v.pathNodeIds.size(); ++i)
            {
                int u = v.pathNodeIds[i];
                int next = v.pathNodeIds[i + 1];

                Vector2 p1 = (static_cast<int>(i) == v.currentPathSegmentIndex) ? Vector2{v.currentPosition.x, v.currentPosition.y} : Vector2{engine.data.graph.getNode(u).position.x, engine.data.graph.getNode(u).position.y};

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
            // Scaling radius to waste volume as specified in prompt
            float baseRadius = 6.0f + std::min(14.0f, fillRatio * 10.0f);

            // Color based on fill & urgency
            Color binColor;
            if (bin.isOverflowing())
            {
                binColor = UIComponents::COLOR_RED;
            }
            else if (fillRatio >= 0.75f)
            {
                binColor = UIComponents::COLOR_GOLD;
            }
            else
            {
                binColor = UIComponents::COLOR_GREEN;
            }

            // Pulsing alert ring for overflowing bins
            if (bin.isOverflowing())
            {
                float pulse = (std::sin(time * 8.0f) + 1.0f) * 0.5f;
                float pulseRadius = baseRadius + 4.0f + pulse * 7.0f;
                DrawCircleLines(static_cast<int>(bin.position.x), static_cast<int>(bin.position.y),
                                pulseRadius, ColorAlpha(UIComponents::COLOR_RED, 0.7f - pulse * 0.4f));
            }

            // Outer rim
            DrawCircle(static_cast<int>(bin.position.x), static_cast<int>(bin.position.y),
                       baseRadius + 2.0f, {20, 24, 34, 220});
            // Core bin circle
            DrawCircle(static_cast<int>(bin.position.x), static_cast<int>(bin.position.y),
                       baseRadius, binColor);

            // Hover highlight
            if (bin.id == hoveredBinId)
            {
                DrawCircleLines(static_cast<int>(bin.position.x), static_cast<int>(bin.position.y),
                                baseRadius + 4.0f, UIComponents::COLOR_CYAN);
            }

            // Labels if enabled
            if (showLabels && camera.camera.zoom >= 0.75f)
            {
                DrawText(bin.name.c_str(),
                         static_cast<int>(bin.position.x - 20.0f),
                         static_cast<int>(bin.position.y + baseRadius + 3.0f),
                         10, UIComponents::TEXT_MUTED);
            }
        }
    }

    void Renderer::renderLandfills()
    {
        for (const auto &lf : engine.data.landfills)
        {
            Vector2 pos = {lf.position.x, lf.position.y};

            // Hexagonal / Square terminal marker
            DrawRectanglePro({pos.x, pos.y, 34.0f, 34.0f}, {17.0f, 17.0f}, 45.0f,
                             lf.id == 0 ? UIComponents::COLOR_CYAN : UIComponents::COLOR_PURPLE);
            DrawRectangleLinesEx({pos.x - 19.0f, pos.y - 19.0f, 38.0f, 38.0f}, 2.0f, WHITE);

            // Landfill Icon / Text
            DrawText(lf.shortName.c_str(), static_cast<int>(pos.x - 28.0f), static_cast<int>(pos.y - 34.0f),
                     12, UIComponents::TEXT_PRIMARY);

            // Mini intake progress bar below landfill
            float p = static_cast<float>(lf.getIntakePercentage());
            Rectangle barRec = {pos.x - 26.0f, pos.y + 22.0f, 52.0f, 7.0f};
            UIComponents::drawProgressBar(barRec, p, p > 80.0f ? UIComponents::COLOR_RED : UIComponents::COLOR_GREEN, nullptr);

            // Queue badge if trucks waiting
            if (lf.queueCount > 0)
            {
                DrawCircle(static_cast<int>(pos.x + 18.0f), static_cast<int>(pos.y - 16.0f), 8.0f, UIComponents::COLOR_GOLD);
                char qBuf[8];
                std::snprintf(qBuf, sizeof(qBuf), "%d", lf.queueCount);
                DrawText(qBuf, static_cast<int>(pos.x + 15.0f), static_cast<int>(pos.y - 21.0f), 10, BLACK);
            }
        }
    }

    void Renderer::renderDepots()
    {
        int nUttara = 2; // Node ID for Uttara Depot
        int nDholai = 3; // Node ID for Dholai Khal Depot

        for (int nodeId : {nUttara, nDholai})
        {
            if (nodeId >= engine.data.graph.getNodeCount())
                continue;
            const auto &node = engine.data.graph.getNode(nodeId);
            Vector2 pos = {node.position.x, node.position.y};

            DrawRectangleRounded({pos.x - 14.0f, pos.y - 14.0f, 28.0f, 28.0f}, 0.2f, 4, {30, 41, 59, 255});
            DrawRectangleRoundedLinesEx({pos.x - 14.0f, pos.y - 14.0f, 28.0f, 28.0f}, 0.2f, 4, 1.5f, UIComponents::COLOR_CYAN);
            DrawText("DEPOT", static_cast<int>(pos.x - 16.0f), static_cast<int>(pos.y - 4.0f), 8, UIComponents::COLOR_CYAN);
        }
    }

    void Renderer::renderVehicles()
    {
        for (const auto &v : engine.data.vehicles)
        {
            Vector2 pos = {v.currentPosition.x, v.currentPosition.y};

            Color truckColor = (v.corporation == Corporation::DNCC) ? UIComponents::COLOR_CYAN : UIComponents::COLOR_GOLD;

            // Draw rotated truck body
            Rectangle truckRec = {pos.x, pos.y, 22.0f, 13.0f};
            Vector2 origin = {11.0f, 6.5f};
            DrawRectanglePro(truckRec, origin, v.headingAngle, truckColor);
            DrawRectangleLinesEx({pos.x - 11.0f, pos.y - 7.0f, 22.0f, 14.0f}, 1.0f, WHITE);

            // Cargo load percentage bar on top
            float loadPct = static_cast<float>(v.getLoadPercentage());
            float barW = 20.0f;
            float barH = 3.5f;
            DrawRectangle(static_cast<int>(pos.x - barW * 0.5f), static_cast<int>(pos.y - 13.0f),
                          static_cast<int>(barW), static_cast<int>(barH), {25, 30, 42, 220});
            DrawRectangle(static_cast<int>(pos.x - barW * 0.5f), static_cast<int>(pos.y - 13.0f),
                          static_cast<int>(barW * (loadPct / 100.0f)), static_cast<int>(barH),
                          loadPct > 85.0f ? UIComponents::COLOR_RED : UIComponents::COLOR_GREEN);

            // State indicator icon
            if (v.state == VehicleState::COLLECTING_WASTE)
            {
                DrawCircle(static_cast<int>(pos.x), static_cast<int>(pos.y - 18.0f), 4.0f, UIComponents::COLOR_GOLD);
            }
            else if (v.state == VehicleState::EN_ROUTE_TO_LANDFILL)
            {
                DrawCircle(static_cast<int>(pos.x), static_cast<int>(pos.y - 18.0f), 4.0f, UIComponents::COLOR_PURPLE);
            }
        }
    }

    void Renderer::renderUI()
    {
        int screenW = GetScreenWidth();
        int screenH = GetScreenHeight();

        // 1. Top Header Bar
        DrawRectangle(0, 0, screenW, 40, UIComponents::PANEL_BG);
        DrawLine(0, 40, screenW, 40, UIComponents::PANEL_BORDER);

        DrawText("OPTIMAL WASTE ROUTING & LANDFILL LOAD-BALANCING SYSTEM (DHAKA)", 18, 12, 14, UIComponents::TEXT_PRIMARY);

        // Simulation Clock & Status
        char clockBuf[64];
        int hours = engine.getHourOfDay();
        int mins = static_cast<int>((engine.simTimeSeconds / 60.0)) % 60;
        std::snprintf(clockBuf, sizeof(clockBuf), "Dhaka Time: %02d:%02d | Speed: %.0fx %s",
                      hours, mins, engine.simSpeedMultiplier,
                      engine.isPaused ? "[PAUSED]" : "[RUNNING]");
        int clockW = MeasureText(clockBuf, 13);
        DrawText(clockBuf, screenW - clockW - 18, 13, 13,
                 engine.isPaused ? UIComponents::COLOR_GOLD : UIComponents::COLOR_GREEN);

        // 2. Metric Cards Row
        float cardW = 185.0f;
        float cardH = 58.0f;
        float startX = 18.0f;
        float startY = 48.0f;

        char valPending[32], valCollected[32], valOverflow[32], valFleet[32];
        std::snprintf(valPending, sizeof(valPending), "%.0f kg", engine.metrics.currentTotalPendingWasteKg);
        std::snprintf(valCollected, sizeof(valCollected), "%.0f kg", engine.metrics.totalWasteCollectedKg);
        std::snprintf(valOverflow, sizeof(valOverflow), "%d bins", engine.metrics.currentOverflowingBins);
        std::snprintf(valFleet, sizeof(valFleet), "%d / %d active", engine.metrics.activeTruckCount,
                      static_cast<int>(engine.data.vehicles.size()));

        UIComponents::drawMetricCard({startX, startY, cardW, cardH},
                                     "PENDING WASTE", valPending, "Bins Accumulating", UIComponents::COLOR_GOLD);

        UIComponents::drawMetricCard({startX + cardW + 10.0f, startY, cardW, cardH},
                                     "TOTAL COLLECTED", valCollected, "Shift Throughput", UIComponents::COLOR_GREEN);

        UIComponents::drawMetricCard({startX + (cardW + 10.0f) * 2.0f, startY, cardW, cardH},
                                     "OVERFLOW RISKS", valOverflow, "Priority Managed",
                                     engine.metrics.currentOverflowingBins > 0 ? UIComponents::COLOR_RED : UIComponents::COLOR_GREEN);

        UIComponents::drawMetricCard({startX + (cardW + 10.0f) * 3.0f, startY, cardW, cardH},
                                     "FLEET DISPATCH", valFleet, "Knapsack Scheduled", UIComponents::COLOR_CYAN);

        // 3. Quick Action Buttons
        float btnX = startX + (cardW + 10.0f) * 4.0f + 10.0f;
        if (UIComponents::drawButton({btnX, startY, 90.0f, 26.0f}, engine.isPaused ? "Play" : "Pause", engine.isPaused))
        {
            engine.isPaused = !engine.isPaused;
        }
        if (UIComponents::drawButton({btnX + 98.0f, startY, 110.0f, 26.0f}, "Disruption [R]"))
        {
            engine.triggerRandomDhakaDisruption();
        }
        if (UIComponents::drawButton({btnX + 216.0f, startY, 80.0f, 26.0f}, "Center [F]"))
        {
            camera.resetView(screenW, screenH);
        }

        // Speed Selector Buttons
        float speedY = startY + 31.0f;
        float spdW = 48.0f;
        if (UIComponents::drawButton({btnX, speedY, spdW, 24.0f}, "1x", engine.simSpeedMultiplier == 1.0f))
            engine.simSpeedMultiplier = 1.0f;
        if (UIComponents::drawButton({btnX + spdW + 4.0f, speedY, spdW, 24.0f}, "2x", engine.simSpeedMultiplier == 2.0f))
            engine.simSpeedMultiplier = 2.0f;
        if (UIComponents::drawButton({btnX + (spdW + 4.0f) * 2.0f, speedY, spdW, 24.0f}, "5x", engine.simSpeedMultiplier == 5.0f))
            engine.simSpeedMultiplier = 5.0f;
        if (UIComponents::drawButton({btnX + (spdW + 4.0f) * 3.0f, speedY, spdW, 24.0f}, "10x", engine.simSpeedMultiplier == 10.0f))
            engine.simSpeedMultiplier = 10.0f;

        // 4. Right Sidebar: Landfill Balancer & Algorithm Inspector
        if (showSidebar)
        {
            float sidebarW = 340.0f;
            float sbX = screenW - sidebarW - 16.0f;
            float sbY = 115.0f;

            // Landfill Load Balancing Card
            UIComponents::drawLandfillBalanceGauge(
                {sbX, sbY, sidebarW, 195.0f},
                static_cast<float>(engine.data.landfills[0].currentIntakeKg),
                static_cast<float>(engine.data.landfills[0].dailyCapacityKg),
                static_cast<float>(engine.data.landfills[1].currentIntakeKg),
                static_cast<float>(engine.data.landfills[1].dailyCapacityKg),
                static_cast<float>(engine.metrics.landfillBalanceRatio));

            // Algorithm Pipeline Inspector
            UIComponents::drawAlgorithmHUD(
                {sbX, sbY + 205.0f, sidebarW, 310.0f},
                engine);

            // Active Disruptions Panel
            float disY = sbY + 525.0f;
            float disH = screenH - disY - 45.0f;
            if (disH > 90.0f)
            {
                UIComponents::drawPanel({sbX, disY, sidebarW, disH}, "Active Dhaka Disruptions");
                float dRowY = disY + 40.0f;
                int count = 0;
                for (auto it = engine.activeDisruptions.rbegin(); it != engine.activeDisruptions.rend(); ++it)
                {
                    if (!it->isActive)
                        continue;
                    char dText[128];
                    std::snprintf(dText, sizeof(dText), "• %s", it->description.c_str());
                    DrawText(dText, static_cast<int>(sbX + 12), static_cast<int>(dRowY), 11,
                             it->type == DisruptionType::ROAD_CLOSURE ? UIComponents::COLOR_RED : UIComponents::COLOR_GOLD);
                    dRowY += 18.0f;
                    if (++count >= 4)
                        break;
                }
                if (count == 0)
                {
                    DrawText("No major traffic gridlock currently reported.",
                             static_cast<int>(sbX + 12), static_cast<int>(dRowY), 11, UIComponents::TEXT_MUTED);
                }
            }
        }

        // 5. Bottom Instructions Bar
        DrawRectangle(0, screenH - 28, screenW, 28, {18, 22, 32, 240});
        DrawLine(0, screenH - 28, screenW, screenH - 28, UIComponents::PANEL_BORDER);
        DrawText("[CLICK ROAD] Congest / Close / Free Road  |  [CLICK BIN] Trigger Waste Surge  |  [R] Random Disruption  |  [F] Center Map  |  [TAB] Toggle HUD",
                 18, screenH - 20, 11, UIComponents::TEXT_MUTED);

        // 6. Interactive Tooltips
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
