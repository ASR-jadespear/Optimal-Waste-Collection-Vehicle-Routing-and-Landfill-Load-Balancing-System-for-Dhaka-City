#include "algorithms/MaxFlow.hpp"
#include "algorithms/AlgorithmTrace.hpp"
#include <queue>
#include <cmath>
#include <limits>
#include <algorithm>
#include <sstream>
#include <iomanip>

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

            if (!visited[sink])
            {
                break;
            }

            double bottleneck = std::numeric_limits<double>::infinity();
            int curr = sink;
            while (curr != source)
            {
                int edgeIdx = parentEdge[curr];
                double residual = edges[edgeIdx].capacity - edges[edgeIdx].flow;
                bottleneck = std::min(bottleneck, residual);
                curr = edges[edgeIdx].u;
            }

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
        bool enforceEqualBalance,
        AlgorithmTrace *traceOut)
    {
        LandfillAssignmentResult result;

        if (traceOut)
        {
            traceOut->clear();
            traceOut->type = TraceType::LANDFILL_MAXFLOW;
            traceOut->title = "Edmonds-Karp Max-Flow Landfill Load Balancing";
        }

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

        double quotaAmin = capAmin;
        double quotaMat = capMat;

        if (enforceEqualBalance && capAmin > 0.0 && capMat > 0.0)
        {
            double idealHalf = totalWaste * 0.55;
            quotaAmin = std::min(capAmin, std::max(idealHalf, 1000.0));
            quotaMat = std::min(capMat, std::max(idealHalf, 1000.0));
        }

        if (traceOut)
        {
            AlgorithmStep step;
            step.type = TraceType::LANDFILL_MAXFLOW;
            step.totalAugmentedFlow = 0.0;
            std::ostringstream ss;
            ss << "Formulated residual flow network: " << numTrucks << " trucks needing disposal ("
               << static_cast<int>(totalWaste) << " kg total). Aminbazar quota: " << static_cast<int>(quotaAmin)
               << " kg, Matuail quota: " << static_cast<int>(quotaMat) << " kg.";
            step.narration = ss.str();
            traceOut->addStep(step);
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

        int pathCount = 0;
        result.totalWasteAllocatedKg = edmondsKarp(source, sink, totalNodes, edges, adj, pathCount);
        result.augmentingPathsCount = pathCount;

        if (result.totalWasteAllocatedKg < totalWaste && (capAmin > quotaAmin || capMat > quotaMat))
        {
            edges[aminToSinkEdge].capacity = capAmin;
            edges[matToSinkEdge].capacity = capMat;
            int secondaryPaths = 0;
            result.totalWasteAllocatedKg += edmondsKarp(source, sink, totalNodes, edges, adj, secondaryPaths);
            result.augmentingPathsCount += secondaryPaths;
        }

        // Trace out augmenting paths inspection
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

            int assignedLF = (flowToAmin > flowToMat) ? 0 : ((flowToMat > flowToAmin) ? 1 : pendingTrucks[i].preferredLandfillId);
            result.truckToLandfillAssignments[truckId] = assignedLF;

            if (assignedLF == 0)
            {
                result.aminbazarAllocatedKg += pendingTrucks[i].wasteKg;
            }
            else
            {
                result.matuailAllocatedKg += pendingTrucks[i].wasteKg;
            }

            if (traceOut)
            {
                AlgorithmStep step;
                step.type = TraceType::LANDFILL_MAXFLOW;
                step.activeVehicleId = truckId;
                step.activeLandfillId = assignedLF;
                step.bottleneckFlow = pendingTrucks[i].wasteKg;
                step.totalAugmentedFlow = result.aminbazarAllocatedKg + result.matuailAllocatedKg;
                step.isBindingEdge = (assignedLF == 0 && result.aminbazarAllocatedKg >= quotaAmin) ||
                                     (assignedLF == 1 && result.matuailAllocatedKg >= quotaMat);

                std::ostringstream ss;
                ss << "Augmenting Path: Source -> Truck " << (truckId + 1) << " ("
                   << static_cast<int>(pendingTrucks[i].wasteKg) << " kg) -> "
                   << (assignedLF == 0 ? "Aminbazar" : "Matuail") << " -> Sink. "
                   << (step.isBindingEdge ? "Landfill quota binding! Balance enforced." : "Flow absorbed.");
                step.narration = ss.str();
                traceOut->addStep(step);
            }
        }

        // Calculate balance ratio
        if (result.aminbazarAllocatedKg > 0.0 && result.matuailAllocatedKg > 0.0)
        {
            double minAlloc = std::min(result.aminbazarAllocatedKg, result.matuailAllocatedKg);
            double maxAlloc = std::max(result.aminbazarAllocatedKg, result.matuailAllocatedKg);
            result.balanceRatio = minAlloc / maxAlloc;
            result.isBalanced = (result.balanceRatio >= 0.70);
        }
        else
        {
            result.balanceRatio = (result.totalWasteAllocatedKg == 0.0) ? 1.0 : 0.0;
            result.isBalanced = false;
        }

        if (traceOut)
        {
            AlgorithmStep step;
            step.type = TraceType::LANDFILL_MAXFLOW;
            step.totalAugmentedFlow = result.totalWasteAllocatedKg;
            std::ostringstream ss;
            ss << "Edmonds-Karp Max-Flow balanced: " << static_cast<int>(result.totalWasteAllocatedKg)
               << " kg routed (" << static_cast<int>(result.aminbazarAllocatedKg) << " kg Aminbazar / "
               << static_cast<int>(result.matuailAllocatedKg) << " kg Matuail). Balance ratio: "
               << std::fixed << std::setprecision(1) << (result.balanceRatio * 100.0) << "% ("
               << (result.isBalanced ? "Optimal 50/50" : "Capacity Constrained") << ").";
            step.narration = ss.str();
            traceOut->addStep(step);
        }

        return result;
    }

} // namespace dhaka
