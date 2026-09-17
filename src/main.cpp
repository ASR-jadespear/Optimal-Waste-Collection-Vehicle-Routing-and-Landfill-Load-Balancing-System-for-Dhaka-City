#include "simulation/SimulationEngine.hpp"
#include "ui/Renderer.hpp"
#include <raylib.h>
#include <iostream>
#include <string>

int main(int argc, char *argv[])
{
    // Check for headless or test mode flag
    bool headlessMode = false;
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--headless" || arg == "--test" || arg == "-t")
        {
            headlessMode = true;
        }
    }

    dhaka::SimulationEngine engine;

    std::cout << "===================================================================\n";
    std::cout << " OPTIMAL WASTE ROUTING & LANDFILL LOAD-BALANCING SYSTEM (DHAKA)   \n";
    std::cout << "===================================================================\n";
    std::cout << "[SYSTEM] Road Nodes: " << engine.data.graph.getNodeCount() << "\n";
    std::cout << "[SYSTEM] Road Edges: " << engine.data.graph.getEdgeCount() << "\n";
    std::cout << "[SYSTEM] Collection Bins: " << engine.data.bins.size() << "\n";
    std::cout << "[SYSTEM] Active Fleet: " << engine.data.vehicles.size() << " vehicles\n";
    std::cout << "[SYSTEM] Landfill Terminals: " << engine.data.landfills.size()
              << " (Aminbazar & Matuail)\n";

    if (headlessMode)
    {
        std::cout << "[SYSTEM] Running in Headless Simulation Mode (300 ticks)...\n";
        for (int step = 0; step < 300; ++step)
        {
            engine.update(1.0f);
        }
        std::cout << "[SYSTEM] Simulated Hours: " << engine.getSimTimeHours() << " hrs\n";
        std::cout << "[SYSTEM] Total Waste Collected: " << engine.metrics.totalWasteCollectedKg << " kg\n";
        std::cout << "[SYSTEM] Completed Trips: " << engine.metrics.completedTripsCount << "\n";
        std::cout << "[SYSTEM] Landfill Balance Ratio: " << engine.metrics.landfillBalanceRatio * 100.0 << "%\n";
        std::cout << "[SYSTEM] Headless run completed successfully.\n";
        return 0;
    }

    // Graphical mode with Raylib
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);

    const int initialWidth = 1440;
    const int initialHeight = 900;

    InitWindow(initialWidth, initialHeight,
               "Optimal Waste Collection Vehicle Routing & Landfill Load-Balancing System (Dhaka City)");

    if (!IsWindowReady())
    {
        std::cerr << "[WARNING] Graphical display could not be opened (e.g. headless session).\n";
        std::cerr << "[TIP] Run with '--headless' or './build/algo_tests' to execute in pure console mode.\n";
        return 1;
    }

    SetTargetFPS(60);

    dhaka::Renderer renderer(engine, initialWidth, initialHeight);

    // Main Simulation Loop
    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        if (dt > 0.1f)
            dt = 0.1f;

        engine.update(dt);
        renderer.update(dt);
        renderer.render();
    }

    CloseWindow();
    std::cout << "[SYSTEM] Simulation terminated cleanly.\n";
    return 0;
}
