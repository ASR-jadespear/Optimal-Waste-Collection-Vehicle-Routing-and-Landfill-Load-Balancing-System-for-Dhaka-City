#pragma once

#include "core/Graph.hpp"
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
        // A* Shortest Path with admissible Euclidean travel-time heuristic
        // Accounts for road closures and dynamic congestion weights
        static PathResult findPathAStar(
            const Graph &graph,
            int startNode,
            int targetNode,
            double maxSpeedKmh = 60.0);

        // Baseline Dijkstra's algorithm for shortest travel time
        static PathResult findPathDijkstra(
            const Graph &graph,
            int startNode,
            int targetNode);

        // Compute all-pairs shortest travel times (or single-source to all nodes)
        static std::vector<double> dijkstraSingleSource(
            const Graph &graph,
            int startNode);
    };

} // namespace dhaka
