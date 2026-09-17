#include "algorithms/MaxFlow.hpp"
#include <queue>
#include <cmath>
#include <limits>
#include <algorithm>

namespace dhaka
{

    void MaxFlow::addFlowEdge(
        int u, int v, double cap,
        std::vector<FlowEdge> &edges,
        std::vector<std::vector<int>> &adj)
    {
        int forwardIdx = static_cast<int>(edges.size());
        int reverseIdx = forwardIdx + 1;

        edges.push_back({u, v, cap, 0.0, reverseIdx});
        edges.push_back({v, u, 0.0, 0.0, forwardIdx});

        adj[u].push_back(forwardIdx);
        adj[v].push_back(reverseIdx);
    }

    double MaxFlow::edmondsKarp(
        int source,
        int sink,
        int totalNodes,
        std::vector<FlowEdge> &edges,
        std::vector<std::vector<int>> &adj,
        int &pathCountOut)
    {
        double totalMaxFlow = 0.0;
        pathCountOut = 0;

        while (true)
        {
            // BFS to find shortest augmenting path in terms of number of edges
            std::vector<int> parentEdge(totalNodes, -1);
            std::vector<bool> visited(totalNodes, false);
            std::queue<int> q;

            q.push(source);
            visited[source] = true;

            while (!q.empty())
            {
                int u = q.front();
                q.pop();

                if (u == sink)
                    break;

                for (int edgeIdx : adj[u])
                {
                    const auto &edge = edges[edgeIdx];
                    int v = edge.v;
                    double residual = edge.capacity - edge.flow;

                    if (!visited[v] && residual > 1e-4)
                    {
                        visited[v] = true;
                        parentEdge[v] = edgeIdx;
                        q.push(v);
                    }
                }
            }

            // If sink not reachable, no more augmenting paths
            if (!visited[sink])
            {
                break;
            }

            // Find bottleneck residual capacity along augmenting path
            double bottleneck = std::numeric_limits<double>::infinity();
            int curr = sink;
            while (curr != source)
            {
                int edgeIdx = parentEdge[curr];
                double residual = edges[edgeIdx].capacity - edges[edgeIdx].flow;
                bottleneck = std::min(bottleneck, residual);
                curr = edges[edgeIdx].u;
            }

            // Augment flow along path
            curr = sink;
            while (curr != source)
            {
                int edgeIdx = parentEdge[curr];
                int revIdx = edges[edgeIdx].residualEdgeIndex;

                edges[edgeIdx].flow += bottleneck;
                edges[revIdx].flow -= bottleneck;

                curr = edges[edgeIdx].u;
            }

            totalMaxFlow += bottleneck;
            pathCountOut++;
        }

        return totalMaxFlow;
    }

    LandfillAssignmentResult MaxFlow::balanceLandfillLoads(
        const std::vector<TruckDemand> &pendingTrucks,
        const Landfill &aminbazar,
        const Landfill &matuail,
        bool enforceEqualBalance)
    {
        LandfillAssignmentResult result;
        if (pendingTrucks.empty())
            return result;

        int numTrucks = static_cast<int>(pendingTrucks.size());
        int source = 0;
        int truckOffset = 1;
        int aminbazarNode = truckOffset + numTrucks;
        int matuailNode = aminbazarNode + 1;
        int sink = matuailNode + 1;
        int totalNodes = sink + 1;

        double totalWaste = 0.0;
        for (const auto &t : pendingTrucks)
        {
            totalWaste += t.wasteKg;
        }

        double capAmin = aminbazar.getRemainingCapacity();
        double capMat = matuail.getRemainingCapacity();

        // Determine target quotas for flow balance
        double quotaAmin = capAmin;
        double quotaMat = capMat;

        if (enforceEqualBalance && capAmin > 0.0 && capMat > 0.0)
        {
            // Enforce balanced split (50% each + buffer for non-divisible trucks)
            double idealHalf = totalWaste * 0.55;
            quotaAmin = std::min(capAmin, std::max(idealHalf, 1000.0));
            quotaMat = std::min(capMat, std::max(idealHalf, 1000.0));
        }

        std::vector<FlowEdge> edges;
        std::vector<std::vector<int>> adj(totalNodes);

        // 1. Source -> Trucks
        for (int i = 0; i < numTrucks; ++i)
        {
            int truckNode = truckOffset + i;
            addFlowEdge(source, truckNode, pendingTrucks[i].wasteKg, edges, adj);
        }

        // 2. Trucks -> Landfills
        for (int i = 0; i < numTrucks; ++i)
        {
            int truckNode = truckOffset + i;
            double w = pendingTrucks[i].wasteKg;

            // Trucks can flow to either landfill, with capacity equal to their load
            if (capAmin > 0.0)
            {
                addFlowEdge(truckNode, aminbazarNode, w, edges, adj);
            }
            if (capMat > 0.0)
            {
                addFlowEdge(truckNode, matuailNode, w, edges, adj);
            }
        }

        // 3. Landfills -> Sink
        int aminToSinkEdge = static_cast<int>(edges.size());
        addFlowEdge(aminbazarNode, sink, quotaAmin, edges, adj);

        int matToSinkEdge = static_cast<int>(edges.size());
        addFlowEdge(matuailNode, sink, quotaMat, edges, adj);

        // Execute Edmonds-Karp
        int pathCount = 0;
        result.totalWasteAllocatedKg = edmondsKarp(source, sink, totalNodes, edges, adj, pathCount);
        result.augmentingPathsCount = pathCount;

        // Check if any trucks were left unallocated due to strict quota, and expand if capacity exists
        if (result.totalWasteAllocatedKg < totalWaste && (capAmin > quotaAmin || capMat > quotaMat))
        {
            // Expand quota to true physical limit for secondary pass
            edges[aminToSinkEdge].capacity = capAmin;
            edges[matToSinkEdge].capacity = capMat;
            int secondaryPaths = 0;
            result.totalWasteAllocatedKg += edmondsKarp(source, sink, totalNodes, edges, adj, secondaryPaths);
            result.augmentingPathsCount += secondaryPaths;
        }

        // Inspect flow on truck->landfill edges to determine assignments
        for (int i = 0; i < numTrucks; ++i)
        {
            int truckNode = truckOffset + i;
            int truckId = pendingTrucks[i].truckId;

            double flowToAmin = 0.0;
            double flowToMat = 0.0;

            for (int edgeIdx : adj[truckNode])
            {
                const auto &e = edges[edgeIdx];
                if (e.v == aminbazarNode)
                {
                    flowToAmin += e.flow;
                }
                else if (e.v == matuailNode)
                {
                    flowToMat += e.flow;
                }
            }

            if (flowToAmin > flowToMat)
            {
                result.truckToLandfillAssignments[truckId] = 0; // 0 = Aminbazar
                result.aminbazarAllocatedKg += pendingTrucks[i].wasteKg;
            }
            else if (flowToMat > flowToAmin)
            {
                result.truckToLandfillAssignments[truckId] = 1; // 1 = Matuail
                result.matuailAllocatedKg += pendingTrucks[i].wasteKg;
            }
            else
            {
                // Tie or fallback: assign to preferred
                int pref = pendingTrucks[i].preferredLandfillId;
                result.truckToLandfillAssignments[truckId] = pref;
                if (pref == 0)
                {
                    result.aminbazarAllocatedKg += pendingTrucks[i].wasteKg;
                }
                else
                {
                    result.matuailAllocatedKg += pendingTrucks[i].wasteKg;
                }
            }
        }

        // Calculate balance ratio
        if (result.aminbazarAllocatedKg > 0.0 && result.matuailAllocatedKg > 0.0)
        {
            double minAlloc = std::min(result.aminbazarAllocatedKg, result.matuailAllocatedKg);
            double maxAlloc = std::max(result.aminbazarAllocatedKg, result.matuailAllocatedKg);
            result.balanceRatio = minAlloc / maxAlloc; // 1.0 is perfectly balanced
            result.isBalanced = (result.balanceRatio >= 0.70);
        }
        else
        {
            result.balanceRatio = (result.totalWasteAllocatedKg == 0.0) ? 1.0 : 0.0;
            result.isBalanced = false;
        }

        return result;
    }

} // namespace dhaka
