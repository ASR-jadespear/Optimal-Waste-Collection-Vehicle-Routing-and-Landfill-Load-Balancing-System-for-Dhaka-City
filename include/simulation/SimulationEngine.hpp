#pragma once

#include "simulation/DhakaMapData.hpp"
#include "algorithms/MergeSort.hpp"
#include "algorithms/PriorityQueue.hpp"
#include "algorithms/KnapsackDP.hpp"
#include "algorithms/GreedyTSP.hpp"
#include "algorithms/Pathfinding.hpp"
#include "algorithms/MaxFlow.hpp"
#include "algorithms/AlgorithmTrace.hpp"
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
        double totalFleetDistanceKm = 0.0;
        double urgencyCoveragePercent = 95.0;
    };

    struct LiveFeedEntry
    {
        double timestamp = 0.0; // sim time in seconds
        std::string message;
        int relevantStage = 0; // 0=RoadNetwork, 1=Routing, 2=Sequencing, 3=LoadSelect, 4=LandfillBalance
    };

    class SimulationEngine
    {
    public:
        SimulationData data;
        SystemMetrics metrics;

        // Simulation Clock & Mode
        double simTimeSeconds = 0.0;
        float simSpeedMultiplier = 1.0f;
        bool isPaused = false;
        bool isLiveMode = false; // Live mode mirrors wall-clock time

        // Algorithmic Data & State
        UrgencyPriorityQueue priorityQueue;
        std::vector<CollectionPoint *> sortedBinsByUrgency;
        KnapsackResult lastKnapsackResult;
        LandfillAssignmentResult lastMaxFlowResult;
        std::string lastAlgorithmStatusMessage;

        // Step-by-Step Algorithm Debugger Trace
        AlgorithmTrace activeTrace;
        void generateTraceForStage(int stage);

        // Live Feed for UI narration
        std::vector<LiveFeedEntry> liveFeedLog;
        void logFeed(const std::string &msg, int stage);

        // Active Disruptions
        std::vector<DisruptionEvent> activeDisruptions;
        int nextDisruptionId = 1;

        // Customization Parameters (Parameters Tab)
        double truckDefaultCapacityKg = 4500.0;
        double wasteGenerationRateMultiplier = 1.0;
        double trafficPeakMultiplier = 2.5;

        SimulationEngine();

        void initialize();
        void reset();

        // Core Tick
        void update(float dtRealSeconds);

        // Manual / Interactive Disruption Triggers (Selective downstream re-triggering)
        void triggerRoadCongestion(int edgeId, double factor = 4.0);
        void triggerRoadClosure(int edgeId);
        void clearRoadDisruption(int edgeId);
        void triggerBinOverflow(int binId, double extraWasteKg = 600.0);
        void triggerRandomDhakaDisruption();

        // Direct Map Editing (Road weights and Dynamic Bins)
        void updateRoadEdge(int edgeId, double speedKmh, double congestion, bool isClosed);
        int addNewBin(const std::string &name, Vec2 pos, double capacityKg = 1500.0, double initialWasteKg = 600.0, Corporation corp = Corporation::COMBINED);
        bool removeBin(int binId);

        // Algorithm Dispatchers
        void runGlobalUrgencyAudit();                 // Uses MergeSort
        void dispatchVehicle(int vehicleId);          // Uses Knapsack + Greedy + A*
        void balanceLandfillsForTruck(int vehicleId); // Uses Edmonds-Karp MaxFlow
        void recalculateVehiclePath(int vehicleId);   // Uses A* to reroute around congestion/closure

        // Status Queries
        double getSimTimeHours() const { return simTimeSeconds / 3600.0; }
        double getHourOfDay() const { return std::fmod(getSimTimeHours(), 24.0); }
        void setSimTimeHours(double hours);

    private:
        void updateBins(double dtHours);
        void updateVehicles(float dtSec);
        void updateDisruptions(double dtHours);
        void updateMetrics();
    };

} // namespace dhaka
