#pragma once

#include "simulation/DhakaMapData.hpp"
#include "algorithms/MergeSort.hpp"
#include "algorithms/PriorityQueue.hpp"
#include "algorithms/KnapsackDP.hpp"
#include "algorithms/GreedyTSP.hpp"
#include "algorithms/Pathfinding.hpp"
#include "algorithms/MaxFlow.hpp"
#include "core/Disruption.hpp"
#include <vector>
#include <string>
#include <memory>

namespace dhaka
{

    struct SystemMetrics
    {
        double totalWasteCollectedKg = 0.0;
        double currentTotalPendingWasteKg = 0.0;
        int totalOverflowIncidents = 0;
        int currentOverflowingBins = 0;
        double averageUrgencyScore = 0.0;
        double aminbazarIntakeKg = 0.0;
        double matuailIntakeKg = 0.0;
        double landfillBalanceRatio = 1.0;
        int activeTruckCount = 0;
        int completedTripsCount = 0;
    };

    class SimulationEngine
    {
    public:
        SimulationData data;
        SystemMetrics metrics;

        // Simulation Clock
        double simTimeSeconds = 0.0;
        float simSpeedMultiplier = 1.0f;
        bool isPaused = false;

        // Algorithmic Data & State
        UrgencyPriorityQueue priorityQueue;
        std::vector<CollectionPoint *> sortedBinsByUrgency;
        KnapsackResult lastKnapsackResult;
        LandfillAssignmentResult lastMaxFlowResult;
        std::string lastAlgorithmStatusMessage;

        // Active Disruptions
        std::vector<DisruptionEvent> activeDisruptions;
        int nextDisruptionId = 1;

        SimulationEngine();

        void initialize();
        void reset();

        // Core Tick
        void update(float dtRealSeconds);

        // Manual / Interactive Disruption Triggers
        void triggerRoadCongestion(int edgeId, double factor = 4.0);
        void triggerRoadClosure(int edgeId);
        void clearRoadDisruption(int edgeId);
        void triggerBinOverflow(int binId, double extraWasteKg = 600.0);
        void triggerRandomDhakaDisruption();

        // Algorithm Dispatchers
        void runGlobalUrgencyAudit();                 // Uses MergeSort
        void dispatchVehicle(int vehicleId);          // Uses Knapsack + Greedy + A*
        void balanceLandfillsForTruck(int vehicleId); // Uses Edmonds-Karp MaxFlow
        void recalculateVehiclePath(int vehicleId);   // Uses A* to reroute around congestion/closure

        // Status Queries
        double getSimTimeHours() const { return simTimeSeconds / 3600.0; }
        int getHourOfDay() const { return static_cast<int>(getSimTimeHours()) % 24; }

    private:
        void updateBins(double dtHours);
        void updateVehicles(float dtSec);
        void updateDisruptions(double dtHours);
        void updateMetrics();
    };

} // namespace dhaka
