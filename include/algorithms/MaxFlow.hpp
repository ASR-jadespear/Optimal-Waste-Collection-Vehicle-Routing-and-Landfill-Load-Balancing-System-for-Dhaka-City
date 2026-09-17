#pragma once

#include "core/Landfill.hpp"
#include "core/Vehicle.hpp"
#include <vector>
#include <unordered_map>

namespace dhaka
{

    struct FlowEdge
    {
        int u, v;
        double capacity;
        double flow;
        int residualEdgeIndex; // Index of reverse edge in adjacency list
    };

    struct TruckDemand
    {
        int truckId;
        double wasteKg;
        int preferredLandfillId; // Preferred based on corporation or proximity
        double travelTimeAminbazarSec;
        double travelTimeMatuailSec;
    };

    struct LandfillAssignmentResult
    {
        double totalWasteAllocatedKg = 0.0;
        double aminbazarAllocatedKg = 0.0;
        double matuailAllocatedKg = 0.0;
        int augmentingPathsCount = 0;
        std::unordered_map<int, int> truckToLandfillAssignments; // truckId -> landfillId (0 for Aminbazar, 1 for Matuail)
        bool isBalanced = true;
        double balanceRatio = 1.0; // aminbazar / matuail or ratio closer to 1.0 = better
    };

    class MaxFlow
    {
    public:
        // Edmonds-Karp Max-Flow algorithm
        // Balances waste intake between Aminbazar (ID 0) and Matuail (ID 1)
        static LandfillAssignmentResult balanceLandfillLoads(
            const std::vector<TruckDemand> &pendingTrucks,
            const Landfill &aminbazar,
            const Landfill &matuail,
            bool enforceEqualBalance = true);

        // Generic Edmonds-Karp algorithm on raw residual network
        static double edmondsKarp(
            int source,
            int sink,
            int totalNodes,
            std::vector<FlowEdge> &edges,
            std::vector<std::vector<int>> &adj,
            int &pathCountOut);

        static void addFlowEdge(
            int u, int v, double cap,
            std::vector<FlowEdge> &edges,
            std::vector<std::vector<int>> &adj);
    };

} // namespace dhaka
