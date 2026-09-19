#pragma once

#include "simulation/SimulationEngine.hpp"
#include "ui/CameraController.hpp"
#include "ui/UIComponents.hpp"
#include <raylib.h>

namespace dhaka
{

    enum class PipelineStage
    {
        ROAD_NETWORK = 0,    // Stage 0: Base G(V,E,t) visualization
        ROUTING = 1,         // Stage 1: Dijkstra / A* pathfinding
        SEQUENCING = 2,      // Stage 2: Greedy TSP visit order
        LOAD_SELECT = 3,     // Stage 3: 0/1 Knapsack DP
        LANDFILL_BALANCE = 4 // Stage 4: Max-Flow Edmonds-Karp
    };

    enum class RightPanelTab
    {
        LIVE_FEED,
        PARAMETERS,
        RESULTS
    };

    struct InlineRoadEditorState
    {
        bool active = false;
        int edgeId = -1;
        Vector2 screenPos = {0, 0};
        float tempSpeed = 40.0f;
        float tempCongestion = 1.0f;
        bool tempClosed = false;
    };

    struct InlineBinEditorState
    {
        bool active = false;
        int binId = -1; // -1 = creating new bin, >= 0 = editing existing bin
        Vec2 worldPos = {0, 0};
        Vector2 screenPos = {0, 0};
        float capacityKg = 1500.0f;
        float initialWasteKg = 600.0f;
    };

    class Renderer
    {
    public:
        SimulationEngine &engine;
        CameraController camera;

        // Pipeline State
        PipelineStage activeStage = PipelineStage::ROAD_NETWORK;
        RightPanelTab activeTab = RightPanelTab::LIVE_FEED;

        // Icon Rail Layer Toggles: [roads, bins, trucks, water]
        bool layerFlags[4] = {true, true, true, true};
        bool showTrafficHeatmap = true;
        bool showEdgeTravelTimes = true;
        bool showNodeIds = false;

        // Left Rail Popover: 0=none, 1=layers, 2=settings, 3=alerts
        int activePopover = 0;

        // Direct Map Inline Editors
        InlineRoadEditorState roadEditor;
        InlineBinEditorState binEditor;

        // Hover inspection
        int hoveredNodeId = -1;
        int hoveredEdgeId = -1;
        int hoveredBinId = -1;
        int hoveredVehicleId = -1;
        int hoveredLandfillId = -1;

        // Disruption injection mode: 0=none, 1=close road, 2=spike congestion, 3=new overflow
        int disruptionMode = 0;

        // Timeline scrubber position [0..1]
        float timelinePos = 0.0f;

        // Animation timers
        float animWaveTimer = 0.0f;

        Renderer(SimulationEngine &engine, int screenWidth = 1440, int screenHeight = 900);

        void update(float dt);
        void render();

    private:
        // Layout rects (computed per-frame from window size)
        Rectangle titleBarRect;
        Rectangle stepperRect;
        Rectangle iconRailRect;
        Rectangle mapRect;
        Rectangle rightPanelRect;
        Rectangle bottomBarRect;

        void computeLayout();

        // Base map rendering (always visible)
        void renderBaseMap();
        void renderWaterways();
        void renderRoads();
        void renderBins();
        void renderLandfills();
        void renderDepots();
        void renderVehicles();
        void renderActivePaths();

        // Stage-specific overlays
        void renderStageOverlay();
        void renderOverlay_RoadNetwork();
        void renderOverlay_Routing();
        void renderOverlay_Sequencing();
        void renderOverlay_LoadSelect();
        void renderOverlay_LandfillBalance();

        // UI Regions
        void renderRightPanel();
        void renderRightPanel_LiveFeed();
        void renderRightPanel_Parameters();
        void renderRightPanel_Results();

        // Popovers and Inline Map Editors
        void renderRailPopovers();
        void renderInlineRoadEditor();
        void renderInlineBinEditor();

        // Interactions
        void handleInteractions();
        void renderTooltips();
    };

} // namespace dhaka
