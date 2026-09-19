#pragma once

#include <vector>
#include <string>

namespace dhaka
{

    enum class TraceType
    {
        ROAD_NETWORK_AUDIT,
        ROUTING_ASTAR,
        SEQUENCING_GREEDY,
        LOAD_SELECT_KNAPSACK,
        LANDFILL_MAXFLOW
    };

    inline const char *traceTypeToString(TraceType type)
    {
        switch (type)
        {
        case TraceType::ROAD_NETWORK_AUDIT:
            return "Road Network";
        case TraceType::ROUTING_ASTAR:
            return "Routing (Dijkstra/A*)";
        case TraceType::SEQUENCING_GREEDY:
            return "Sequencing (Greedy)";
        case TraceType::LOAD_SELECT_KNAPSACK:
            return "Load Selection (0/1 Knapsack)";
        case TraceType::LANDFILL_MAXFLOW:
            return "Landfill Balance (Max-Flow)";
        default:
            return "Algorithm";
        }
    }

    struct AlgorithmStep
    {
        TraceType type = TraceType::ROUTING_ASTAR;
        int stepNumber = 0;
        int totalSteps = 0;
        std::string narration; // Plain language narration for non-technical management

        // Map visual focus
        int activeNodeId = -1;
        int targetNodeId = -1;
        int activeEdgeId = -1;
        int activeBinId = -1;
        int activeVehicleId = -1;
        int activeLandfillId = -1;

        // Routing (A*/Dijkstra)
        std::vector<int> frontierNodeIds;    // Nodes in open set
        std::vector<int> settledNodeIds;     // Nodes in closed set
        std::vector<int> currentPathNodeIds; // Path found so far
        double currentCost = 0.0;
        double heuristicCost = 0.0;

        // Sequencing (Greedy TSP)
        std::vector<int> tourBinIdsSoFar;
        int candidateBinId = -1;
        double candidateScore = 0.0;

        // Knapsack DP
        std::vector<int> knapsackSelectedBins;
        std::vector<int> knapsackRejectedBins;
        int evaluatingItemIndex = -1;
        double payloadWeightKg = 0.0;
        double remainingCapacityKg = 0.0;
        int accumulatedValue = 0;
        bool itemKept = false;

        // Max-Flow (Edmonds-Karp)
        std::vector<int> augmentingPath;
        double bottleneckFlow = 0.0;
        double totalAugmentedFlow = 0.0;
        bool isBindingEdge = false;
    };

    struct AlgorithmTrace
    {
        TraceType type = TraceType::ROUTING_ASTAR;
        std::string title;
        std::vector<AlgorithmStep> steps;
        int currentStepIndex = 0;
        bool isPlaying = false;
        float playTimer = 0.0f;
        float stepIntervalSec = 0.6f;

        void clear()
        {
            steps.clear();
            currentStepIndex = 0;
            isPlaying = false;
            playTimer = 0.0f;
        }

        void addStep(const AlgorithmStep &step)
        {
            steps.push_back(step);
            steps.back().stepNumber = static_cast<int>(steps.size());
            for (auto &s : steps)
            {
                s.totalSteps = static_cast<int>(steps.size());
            }
        }

        void stepForward()
        {
            if (!steps.empty() && currentStepIndex < static_cast<int>(steps.size()) - 1)
            {
                currentStepIndex++;
            }
            else
            {
                isPlaying = false;
            }
        }

        void stepBackward()
        {
            if (currentStepIndex > 0)
            {
                currentStepIndex--;
            }
        }

        void resetToStart()
        {
            currentStepIndex = 0;
        }

        void goToEnd()
        {
            if (!steps.empty())
            {
                currentStepIndex = static_cast<int>(steps.size()) - 1;
            }
        }

        const AlgorithmStep *getCurrentStep() const
        {
            if (steps.empty() || currentStepIndex < 0 || currentStepIndex >= static_cast<int>(steps.size()))
            {
                return nullptr;
            }
            return &steps[currentStepIndex];
        }

        void update(float dt)
        {
            if (!isPlaying || steps.empty())
                return;

            playTimer += dt;
            if (playTimer >= stepIntervalSec)
            {
                playTimer = 0.0f;
                stepForward();
            }
        }
    };

} // namespace dhaka
