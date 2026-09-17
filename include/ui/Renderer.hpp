#pragma once

#include "simulation/SimulationEngine.hpp"
#include "ui/CameraController.hpp"
#include "ui/UIComponents.hpp"
#include <raylib.h>

namespace dhaka
{

    class Renderer
    {
    public:
        SimulationEngine &engine;
        CameraController camera;

        // View Options
        bool showPaths = true;
        bool showLabels = true;
        bool showHeatmap = false;
        bool showSidebar = true;

        // Hover inspection
        int hoveredNodeId = -1;
        int hoveredEdgeId = -1;
        int hoveredBinId = -1;
        int hoveredVehicleId = -1;
        int hoveredLandfillId = -1;

        Renderer(SimulationEngine &engine, int screenWidth = 1440, int screenHeight = 900);

        void update(float dt);
        void render();

    private:
        void renderWorld();
        void renderWaterways();
        void renderRoads();
        void renderBins();
        void renderLandfills();
        void renderDepots();
        void renderVehicles();
        void renderActivePaths();

        void renderUI();
        void handleInteractions();
    };

} // namespace dhaka
