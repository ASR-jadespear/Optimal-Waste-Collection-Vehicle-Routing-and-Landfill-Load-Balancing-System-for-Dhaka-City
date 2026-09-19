#pragma once

#include "core/Graph.hpp"
#include "algorithms/AlgorithmTrace.hpp"
#include <vector>

namespace dhaka
{

    struct PathResult
    {
        bool found = false;
        std::vector<int> nodePath;
        std::vector<int> edgePath;
        double totalTravelTimeSeconds = 0.0;
        double totalDistanceMeters = 0.0;
        int nodesExplored = 0;
    };

    class Pathfinding
    {
    public:
        // A* Shortest Path with admissible Euclidean travel-time heuristic,
        // time-varying diurnal congestion weights, and step-through tracing
        static PathResult findPathAStar(
            const Graph &graph,
            int startNode,
            int targetNode,
            double maxSpeedKmh = 60.0,
            double hourOfDay = 12.0,
            AlgorithmTrace *traceOut = nullptr);

        // Baseline Dijkstra's algorithm for shortest travel time
        static PathResult findPathDijkstra(
            const Graph &graph,
            int startNode,
            int targetNode,
            double hourOfDay = 12.0,
            AlgorithmTrace *traceOut = nullptr);

        // Compute all-pairs shortest travel times (or single-source to all nodes)
        static std::vector<double> dijkstraSingleSource(
            const Graph &graph,
            int startNode,
            double hourOfDay = 12.0);
    };

} // namespace dhaka
