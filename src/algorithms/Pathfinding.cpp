#include "algorithms/Pathfinding.hpp"
#include <queue>
#include <limits>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace dhaka
{

    struct AStarNode
    {
        int id;
        double gCost; // Exact cost so far
        double fCost; // gCost + heuristic

        bool operator>(const AStarNode &o) const
        {
            return fCost > o.fCost;
        }
    };

    PathResult Pathfinding::findPathAStar(
        const Graph &graph,
        int startNode,
        int targetNode,
        double maxSpeedKmh,
        double hourOfDay,
        AlgorithmTrace *traceOut)
    {
        PathResult res;
        int n = graph.getNodeCount();

        if (startNode < 0 || startNode >= n || targetNode < 0 || targetNode >= n)
        {
            return res;
        }

        if (traceOut)
        {
            traceOut->clear();
            traceOut->type = TraceType::ROUTING_ASTAR;
            std::ostringstream titleSs;
            titleSs << "A* Shortest Path: Node " << startNode << " -> Node " << targetNode;
            traceOut->title = titleSs.str();
        }

        if (startNode == targetNode)
        {
            res.found = true;
            res.nodePath = {startNode};
            if (traceOut)
            {
                AlgorithmStep step;
                step.type = TraceType::ROUTING_ASTAR;
                step.activeNodeId = startNode;
                step.targetNodeId = targetNode;
                step.settledNodeIds = {startNode};
                step.currentPathNodeIds = {startNode};
                step.narration = "Start and target nodes are identical. Vehicle is already at destination.";
                traceOut->addStep(step);
            }
            return res;
        }

        double maxSpeedMs = (maxSpeedKmh * 1000.0) / 3600.0;
        const Vec2 &targetPos = graph.getNode(targetNode).position;

        auto heuristic = [&](int u) -> double
        {
            const Vec2 &uPos = graph.getNode(u).position;
            double distMeters = uPos.distanceTo(targetPos) * 20.0; // Scaled meters
            return distMeters / maxSpeedMs;
        };

        std::vector<double> gCost(n, std::numeric_limits<double>::infinity());
        std::vector<int> parentNode(n, -1);
        std::vector<int> parentEdge(n, -1);
        std::vector<bool> closed(n, false);

        std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> openSet;

        gCost[startNode] = 0.0;
        double startH = heuristic(startNode);
        openSet.push({startNode, 0.0, startH});

        if (traceOut)
        {
            AlgorithmStep step;
            step.type = TraceType::ROUTING_ASTAR;
            step.activeNodeId = startNode;
            step.targetNodeId = targetNode;
            step.frontierNodeIds = {startNode};
            step.currentCost = 0.0;
            step.heuristicCost = startH;
            std::ostringstream ss;
            ss << "Initialized A* from Node #" << startNode << " (" << graph.getNode(startNode).name
               << ") targeting Node #" << targetNode << " (" << graph.getNode(targetNode).name
               << "). Initial heuristic h(s) = " << std::fixed << std::setprecision(1) << startH << "s.";
            step.narration = ss.str();
            traceOut->addStep(step);
        }

        while (!openSet.empty())
        {
            AStarNode current = openSet.top();
            openSet.pop();

            int u = current.id;
            if (closed[u])
                continue;
            closed[u] = true;
            res.nodesExplored++;

            // Gather closed and open lists for snapshot
            std::vector<int> closedList;
            for (int i = 0; i < n; ++i)
            {
                if (closed[i])
                    closedList.push_back(i);
            }

            if (traceOut && traceOut->steps.size() < 40)
            {
                AlgorithmStep step;
                step.type = TraceType::ROUTING_ASTAR;
                step.activeNodeId = u;
                step.targetNodeId = targetNode;
                step.settledNodeIds = closedList;
                step.currentCost = gCost[u];
                step.heuristicCost = heuristic(u);
                std::ostringstream ss;
                ss << "Popped Node #" << u << " (" << graph.getNode(u).name << ") from priority queue. "
                   << "Elapsed g = " << std::fixed << std::setprecision(1) << gCost[u] << "s, "
                   << "estimated remaining h = " << heuristic(u) << "s (f = " << (gCost[u] + heuristic(u)) << "s).";
                step.narration = ss.str();
                traceOut->addStep(step);
            }

            if (u == targetNode)
            {
                res.found = true;
                break;
            }

            for (int edgeId : graph.getOutgoingEdges(u))
            {
                const auto &edge = graph.getEdge(edgeId);

                // Skip closed roads
                if (edge.isClosed)
                    continue;

                int v = edge.toNode;
                if (closed[v])
                    continue;

                double edgeTravelTime = edge.getTravelTimeSeconds(hourOfDay);
                if (std::isinf(edgeTravelTime))
                    continue;

                double tentativeG = gCost[u] + edgeTravelTime;

                if (tentativeG < gCost[v])
                {
                    gCost[v] = tentativeG;
                    parentNode[v] = u;
                    parentEdge[v] = edgeId;
                    double h = heuristic(v);
                    double f = tentativeG + h;
                    openSet.push({v, tentativeG, f});

                    if (traceOut && traceOut->steps.size() < 40)
                    {
                        AlgorithmStep step;
                        step.type = TraceType::ROUTING_ASTAR;
                        step.activeNodeId = u;
                        step.targetNodeId = targetNode;
                        step.activeEdgeId = edgeId;
                        step.settledNodeIds = closedList;
                        step.currentCost = tentativeG;
                        step.heuristicCost = h;
                        std::ostringstream ss;
                        ss << "Relaxed edge (" << edge.roadName << ") to Node #" << v
                           << ": new travel time g = " << std::fixed << std::setprecision(1) << tentativeG
                           << "s (congestion " << edge.congestionFactor << "x).";
                        step.narration = ss.str();
                        traceOut->addStep(step);
                    }
                }
            }
        }

        if (!res.found)
            return res;

        // Reconstruct path backwards
        int curr = targetNode;
        while (curr != -1)
        {
            res.nodePath.push_back(curr);
            int edgeId = parentEdge[curr];
            if (edgeId != -1)
            {
                res.edgePath.push_back(edgeId);
                res.totalTravelTimeSeconds += graph.getEdge(edgeId).getTravelTimeSeconds(hourOfDay);
                res.totalDistanceMeters += graph.getEdge(edgeId).lengthMeters;
            }
            curr = parentNode[curr];
        }

        std::reverse(res.nodePath.begin(), res.nodePath.end());
        std::reverse(res.edgePath.begin(), res.edgePath.end());

        if (traceOut)
        {
            AlgorithmStep step;
            step.type = TraceType::ROUTING_ASTAR;
            step.activeNodeId = targetNode;
            step.targetNodeId = targetNode;
            step.settledNodeIds = res.nodePath;
            step.currentPathNodeIds = res.nodePath;
            step.currentCost = res.totalTravelTimeSeconds;
            std::ostringstream ss;
            ss << "A* converged! Optimal path settled across " << res.nodePath.size()
               << " intersections. Total estimated travel time: "
               << std::fixed << std::setprecision(1) << res.totalTravelTimeSeconds << "s ("
               << static_cast<int>(res.totalDistanceMeters) << "m).";
            step.narration = ss.str();
            traceOut->addStep(step);
        }

        return res;
    }

    PathResult Pathfinding::findPathDijkstra(
        const Graph &graph,
        int startNode,
        int targetNode,
        double hourOfDay,
        AlgorithmTrace *traceOut)
    {
        PathResult res;
        int n = graph.getNodeCount();

        if (startNode < 0 || startNode >= n || targetNode < 0 || targetNode >= n)
        {
            return res;
        }

        if (startNode == targetNode)
        {
            res.found = true;
            res.nodePath = {startNode};
            return res;
        }

        std::vector<double> dist(n, std::numeric_limits<double>::infinity());
        std::vector<int> parentNode(n, -1);
        std::vector<int> parentEdge(n, -1);
        std::vector<bool> visited(n, false);

        using PDI = std::pair<double, int>; // (dist, node)
        std::priority_queue<PDI, std::vector<PDI>, std::greater<PDI>> pq;

        dist[startNode] = 0.0;
        pq.push({0.0, startNode});

        while (!pq.empty())
        {
            auto [d, u] = pq.top();
            pq.pop();

            if (visited[u])
                continue;
            visited[u] = true;
            res.nodesExplored++;

            if (u == targetNode)
            {
                res.found = true;
                break;
            }

            for (int edgeId : graph.getOutgoingEdges(u))
            {
                const auto &edge = graph.getEdge(edgeId);
                if (edge.isClosed)
                    continue;

                int v = edge.toNode;
                if (visited[v])
                    continue;

                double edgeCost = edge.getTravelTimeSeconds(hourOfDay);
                if (std::isinf(edgeCost))
                    continue;

                if (dist[u] + edgeCost < dist[v])
                {
                    dist[v] = dist[u] + edgeCost;
                    parentNode[v] = u;
                    parentEdge[v] = edgeId;
                    pq.push({dist[v], v});
                }
            }
        }

        if (!res.found)
            return res;

        int curr = targetNode;
        while (curr != -1)
        {
            res.nodePath.push_back(curr);
            int edgeId = parentEdge[curr];
            if (edgeId != -1)
            {
                res.edgePath.push_back(edgeId);
                res.totalTravelTimeSeconds += graph.getEdge(edgeId).getTravelTimeSeconds(hourOfDay);
                res.totalDistanceMeters += graph.getEdge(edgeId).lengthMeters;
            }
            curr = parentNode[curr];
        }

        std::reverse(res.nodePath.begin(), res.nodePath.end());
        std::reverse(res.edgePath.begin(), res.edgePath.end());

        return res;
    }

    std::vector<double> Pathfinding::dijkstraSingleSource(
        const Graph &graph,
        int startNode,
        double hourOfDay)
    {
        int n = graph.getNodeCount();
        std::vector<double> dist(n, std::numeric_limits<double>::infinity());
        if (startNode < 0 || startNode >= n)
            return dist;

        std::vector<bool> visited(n, false);
        using PDI = std::pair<double, int>;
        std::priority_queue<PDI, std::vector<PDI>, std::greater<PDI>> pq;

        dist[startNode] = 0.0;
        pq.push({0.0, startNode});

        while (!pq.empty())
        {
            auto [d, u] = pq.top();
            pq.pop();

            if (visited[u])
                continue;
            visited[u] = true;

            for (int edgeId : graph.getOutgoingEdges(u))
            {
                const auto &edge = graph.getEdge(edgeId);
                if (edge.isClosed)
                    continue;

                int v = edge.toNode;
                if (visited[v])
                    continue;

                double cost = edge.getTravelTimeSeconds(hourOfDay);
                if (std::isinf(cost))
                    continue;

                if (dist[u] + cost < dist[v])
                {
                    dist[v] = dist[u] + cost;
                    pq.push({dist[v], v});
                }
            }
        }

        return dist;
    }

} // namespace dhaka
