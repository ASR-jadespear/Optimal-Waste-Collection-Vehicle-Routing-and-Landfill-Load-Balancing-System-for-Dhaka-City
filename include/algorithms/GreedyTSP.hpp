#pragma once

#include "core/CollectionPoint.hpp"
#include "core/Graph.hpp"
#include "algorithms/AlgorithmTrace.hpp"
#include <vector>

namespace dhaka
{

    class GreedyTSP
    {
    public:
        // Sequences the visit order of the knapsack-selected bins using
        // Priority-Based Nearest Neighbour heuristic.
        // u: Current truck location node
        // selectedBinIds: Subset of bin IDs chosen by Knapsack
        // allBins: Master list of collection points
        // graph: Road network graph for distance calculations
        // alpha: Urgency weight exponent (higher = focus more on urgency)
        // beta: Distance/time penalty exponent (higher = focus more on nearest)
        static std::vector<int> sequenceVisits(
            int startNodeId,
            const std::vector<int> &selectedBinIds,
            const std::vector<CollectionPoint> &allBins,
            const Graph &graph,
            double alpha = 1.5,
            double beta = 1.0,
            AlgorithmTrace *traceOut = nullptr,
            int vehicleId = -1);
    };

} // namespace dhaka
