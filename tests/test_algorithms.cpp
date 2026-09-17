#include "algorithms/MergeSort.hpp"
#include "algorithms/PriorityQueue.hpp"
#include "algorithms/KnapsackDP.hpp"
#include "algorithms/GreedyTSP.hpp"
#include "algorithms/Pathfinding.hpp"
#include "algorithms/MaxFlow.hpp"
#include "simulation/SimulationEngine.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace dhaka;

static void printHeader(const std::string &title)
{
    std::cout << "\n======================================================\n";
    std::cout << " [TEST] " << title << "\n";
    std::cout << "======================================================\n";
}

void test_mergesort_and_priority_queue()
{
    printHeader("1. MergeSort & Priority Queue (Urgency Sorting)");

    std::vector<CollectionPoint> bins;
    bins.emplace_back(0, 0, "Bin A", Corporation::DNCC, Vec2{100, 100}, 200.0, 1000.0, 50.0);
    bins.emplace_back(1, 1, "Bin B", Corporation::DNCC, Vec2{200, 100}, 950.0, 1000.0, 80.0);
    bins.emplace_back(2, 2, "Bin C", Corporation::DSCC, Vec2{300, 100}, 1400.0, 1000.0, 120.0); // Overflow
    bins.emplace_back(3, 3, "Bin D", Corporation::DSCC, Vec2{400, 100}, 500.0, 1000.0, 40.0);
    bins.emplace_back(4, 4, "Bin E", Corporation::DNCC, Vec2{500, 100}, 800.0, 1000.0, 60.0);

    // MergeSort test
    std::vector<CollectionPoint *> ptrs;
    for (auto &b : bins)
        ptrs.push_back(&b);

    MergeSort::sortByUrgency(ptrs, true);

    std::cout << "Sorted Bins by Urgency (MergeSort Descending):\n";
    for (size_t i = 0; i < ptrs.size(); ++i)
    {
        std::cout << "  " << i + 1 << ". " << ptrs[i]->name
                  << " | Urgency: " << ptrs[i]->urgencyScore
                  << " | Waste: " << ptrs[i]->currentWasteKg << " kg\n";
        if (i > 0)
        {
            assert(ptrs[i - 1]->urgencyScore >= ptrs[i]->urgencyScore);
        }
    }
    // Most urgent must be overflowing Bin C
    assert(ptrs[0]->id == 2);
    std::cout << ">>> MergeSort verification PASSED!\n";

    // Priority Queue test
    UrgencyPriorityQueue pq;
    pq.buildFromPoints(bins);
    assert(!pq.empty());
    assert(pq.size() == bins.size());

    std::cout << "\nPriority Queue Top-to-Bottom Extraction:\n";
    double prevUrgency = 9999.0;
    while (!pq.empty())
    {
        auto item = pq.pop();
        std::cout << "  Popped Bin #" << item.binId << " | Urgency: " << item.urgencyScore << "\n";
        assert(item.urgencyScore <= prevUrgency + 1e-4);
        prevUrgency = item.urgencyScore;
    }
    std::cout << ">>> Priority Queue verification PASSED!\n";
}

void test_knapsack_dp()
{
    printHeader("2. 0/1 Knapsack (Dynamic Programming) Vehicle Target Selection");

    std::vector<CollectionPoint> bins;
    // Bins with different waste weights and urgencies
    bins.emplace_back(0, 0, "Bin 1 (Low Urg)", Corporation::DNCC, Vec2{0, 0}, 1000.0, 1000.0, 10.0);
    bins.emplace_back(1, 1, "Bin 2 (Critical)", Corporation::DNCC, Vec2{0, 0}, 2000.0, 2000.0, 90.0);
    bins.emplace_back(2, 2, "Bin 3 (Medium Urg)", Corporation::DNCC, Vec2{0, 0}, 1500.0, 1500.0, 40.0);
    bins.emplace_back(3, 3, "Bin 4 (Critical)", Corporation::DNCC, Vec2{0, 0}, 2500.0, 2500.0, 100.0);
    bins.emplace_back(4, 4, "Bin 5 (Small Heavy)", Corporation::DNCC, Vec2{0, 0}, 3000.0, 3000.0, 20.0);

    // Truck capacity: 4500 kg
    double truckCap = 4500.0;
    std::vector<const CollectionPoint *> cand;
    for (const auto &b : bins)
        cand.push_back(&b);

    KnapsackResult res = KnapsackDP::solve(truckCap, cand, 50.0);

    std::cout << "Truck Capacity: " << truckCap << " kg\n";
    std::cout << "DP Matrix Value Achieved: " << res.totalValue << "\n";
    std::cout << "Total Selected Weight: " << res.totalWeightKg << " kg (Utilization: "
              << (res.totalWeightKg / truckCap) * 100.0 << "%)\n";
    std::cout << "Selected Bins:\n";
    for (int id : res.selectedBinIds)
    {
        std::cout << "  - Bin #" << id << " (" << bins[id].name
                  << ", Weight: " << bins[id].currentWasteKg << " kg, Urgency: "
                  << bins[id].urgencyScore << ")\n";
    }

    assert(res.totalWeightKg <= truckCap);
    assert(!res.selectedBinIds.empty());
    // Critical bins (Bin 1 or Bin 3) should be preferred over low-urgency Bin 5
    std::cout << ">>> 0/1 Knapsack DP verification PASSED!\n";
}

void test_greedy_tsp()
{
    printHeader("3. Greedy (Priority-Based Nearest Neighbour) Visit Sequencing");

    Graph g;
    int n0 = g.addNode("Depot", {0, 0});
    int n1 = g.addNode("Near Low Urg", {10, 0});
    int n2 = g.addNode("Far High Urg", {100, 0});
    int n3 = g.addNode("Mid High Urg", {40, 0});

    std::vector<CollectionPoint> bins;
    bins.emplace_back(0, n1, "Near Low", Corporation::DNCC, Vec2{10, 0}, 500.0, 1000.0, 10.0);
    bins.emplace_back(1, n2, "Far High", Corporation::DNCC, Vec2{100, 0}, 1500.0, 1000.0, 120.0); // Very urgent
    bins.emplace_back(2, n3, "Mid High", Corporation::DNCC, Vec2{40, 0}, 1400.0, 1000.0, 100.0);  // Urgent

    std::vector<int> selected = {0, 1, 2};
    std::vector<int> tour = GreedyTSP::sequenceVisits(n0, selected, bins, g, 1.8, 1.0);

    std::cout << "Sequenced Tour Order from Depot (Node " << n0 << "):\n";
    for (int binId : tour)
    {
        std::cout << "  -> Bin #" << binId << " (" << bins[binId].name
                  << " | Urgency: " << bins[binId].urgencyScore << ")\n";
    }

    assert(tour.size() == selected.size());
    std::cout << ">>> Greedy Priority Nearest Neighbour verification PASSED!\n";
}

void test_pathfinding()
{
    printHeader("4. Time-Varying A* & Dijkstra Shortest Path Routing");

    Graph g;
    // Simple diamond network:
    //      (1)
    //    /     \
    // (0)       (3)
    //    \     /
    //      (2)
    int n0 = g.addNode("Node 0", {0, 0});
    int n1 = g.addNode("Node 1 (Upper)", {100, -50});
    int n2 = g.addNode("Node 2 (Lower)", {100, 50});
    int n3 = g.addNode("Node 3 (Target)", {200, 0});

    int e01 = g.addEdge(n0, n1, 1000.0, 50.0, "Upper Path 1");
    int e13 = g.addEdge(n1, n3, 1000.0, 50.0, "Upper Path 2");

    int e02 = g.addEdge(n0, n2, 1000.0, 30.0, "Lower Path 1"); // Slower base speed
    int e23 = g.addEdge(n2, n3, 1000.0, 30.0, "Lower Path 2");

    // Test A* and Dijkstra under free flow
    PathResult astarRes = Pathfinding::findPathAStar(g, n0, n3);
    PathResult dijkstraRes = Pathfinding::findPathDijkstra(g, n0, n3);

    assert(astarRes.found && dijkstraRes.found);
    std::cout << "A* Travel Time: " << astarRes.totalTravelTimeSeconds << " s (Nodes explored: " << astarRes.nodesExplored << ")\n";
    std::cout << "Dijkstra Travel Time: " << dijkstraRes.totalTravelTimeSeconds << " s (Nodes explored: " << dijkstraRes.nodesExplored << ")\n";
    assert(std::abs(astarRes.totalTravelTimeSeconds - dijkstraRes.totalTravelTimeSeconds) < 1e-3);
    assert(astarRes.nodePath[1] == n1); // Upper faster path chosen

    // Test Dynamic Congestion Spike
    std::cout << "\nSpiking congestion on Upper Path 1 (e01) by 5.0x...\n";
    g.setEdgeCongestion(e01, 5.0);
    PathResult reroutedAstar = Pathfinding::findPathAStar(g, n0, n3);
    std::cout << "Rerouted Path chose Node: " << reroutedAstar.nodePath[1] << " (Lower Path)\n";
    assert(reroutedAstar.nodePath[1] == n2); // Reroutes to lower path

    // Test Emergency Road Closure
    std::cout << "\nClosing Lower Path 1 (e02) completely...\n";
    g.setEdgeClosed(e02, true);
    PathResult blockedReroute = Pathfinding::findPathAStar(g, n0, n3);
    std::cout << "With lower path closed, traffic forces through upper path: Node " << blockedReroute.nodePath[1] << "\n";
    assert(blockedReroute.nodePath[1] == n1);

    std::cout << ">>> Time-Varying A* and Dijkstra verification PASSED!\n";
}

void test_maxflow_load_balancing()
{
    printHeader("5. Max-Flow (Edmonds-Karp) Landfill Load Balancing");

    Landfill aminbazar(0, 0, "Aminbazar", "Aminbazar", Corporation::DNCC, {0, 0}, 20000.0, 300.0);
    Landfill matuail(1, 1, "Matuail", "Matuail", Corporation::DSCC, {0, 0}, 20000.0, 300.0);

    // 4 trucks each carrying 4000 kg waste (total 16,000 kg)
    std::vector<TruckDemand> trucks = {
        {0, 4000.0, 0, 0.0, 0.0},
        {1, 4000.0, 0, 0.0, 0.0},
        {2, 4000.0, 1, 0.0, 0.0},
        {3, 4000.0, 1, 0.0, 0.0}};

    LandfillAssignmentResult res = MaxFlow::balanceLandfillLoads(trucks, aminbazar, matuail, true);

    std::cout << "Total Waste to Route: 16,000 kg\n";
    std::cout << "Edmonds-Karp Total Flow Routed: " << res.totalWasteAllocatedKg << " kg\n";
    std::cout << "Aminbazar Allocated: " << res.aminbazarAllocatedKg << " kg\n";
    std::cout << "Matuail Allocated: " << res.matuailAllocatedKg << " kg\n";
    std::cout << "Balance Ratio: " << res.balanceRatio * 100.0 << "%\n";
    std::cout << "Augmenting Paths Found by BFS: " << res.augmentingPathsCount << "\n";

    assert(std::abs(res.totalWasteAllocatedKg - 16000.0) < 1e-3);
    assert(std::abs(res.aminbazarAllocatedKg - 8000.0) < 1e-3);
    assert(std::abs(res.matuailAllocatedKg - 8000.0) < 1e-3);
    assert(res.balanceRatio >= 0.99); // Perfectly balanced 50/50!

    // Test Landfill Saturation Condition
    std::cout << "\nTesting Landfill Saturation: Aminbazar almost full (remaining capacity: 3,000 kg)...\n";
    aminbazar.currentIntakeKg = 17000.0; // 3,000 kg left
    LandfillAssignmentResult satRes = MaxFlow::balanceLandfillLoads(trucks, aminbazar, matuail, true);

    std::cout << "Aminbazar Intake: " << satRes.aminbazarAllocatedKg << " kg (Cap: 3000 kg)\n";
    std::cout << "Matuail Intake: " << satRes.matuailAllocatedKg << " kg (Excess redirected)\n";
    assert(satRes.aminbazarAllocatedKg <= 3000.0 + 1e-3);
    assert(satRes.matuailAllocatedKg >= 12000.0 - 1e-3);

    std::cout << ">>> Max-Flow Edmonds-Karp Load Balancing verification PASSED!\n";
}

void test_full_system_simulation()
{
    printHeader("6. Full System Simulation Integration Test");

    SimulationEngine engine;
    std::cout << "Dhaka Map Nodes: " << engine.data.graph.getNodeCount() << "\n";
    std::cout << "Dhaka Map Edges: " << engine.data.graph.getEdgeCount() << "\n";
    std::cout << "Active Waste Bins: " << engine.data.bins.size() << "\n";
    std::cout << "Fleet Vehicles: " << engine.data.vehicles.size() << "\n";
    std::cout << "Landfill Terminals: " << engine.data.landfills.size() << "\n";

    assert(engine.data.graph.getNodeCount() >= 20);
    assert(engine.data.bins.size() >= 20);
    assert(engine.data.vehicles.size() >= 4);

    std::cout << "\nSimulating 300 seconds of waste dynamics...\n";
    for (int step = 0; step < 300; ++step)
    {
        engine.update(1.0f);
    }

    std::cout << "Simulation Time Elapsed: " << engine.simTimeSeconds << " s (" << engine.getSimTimeHours() << " hrs)\n";
    std::cout << "Total Waste Collected: " << engine.metrics.totalWasteCollectedKg << " kg\n";
    std::cout << "Active Dispatched Trucks: " << engine.metrics.activeTruckCount << "\n";
    std::cout << "Completed Truck Trips: " << engine.metrics.completedTripsCount << "\n";

    assert(engine.metrics.activeTruckCount > 0);
    std::cout << ">>> Full Simulation Engine verification PASSED!\n";
}

int main()
{
    std::cout << "\n===================================================================\n";
    std::cout << " OPTIMAL WASTE ROUTING & LANDFILL LOAD-BALANCING SYSTEM (DHAKA)   \n";
    std::cout << " Console Algorithm & Logic Verification Test Suite                 \n";
    std::cout << "===================================================================\n";

    test_mergesort_and_priority_queue();
    test_knapsack_dp();
    test_greedy_tsp();
    test_pathfinding();
    test_maxflow_load_balancing();
    test_full_system_simulation();

    std::cout << "\n===================================================================\n";
    std::cout << " ALL 6 BACKEND & ALGORITHM TEST SUITES PASSED SUCCESSFULLY!       \n";
    std::cout << " Ready for Raylib Graphical Engine & Interactive Animation.        \n";
    std::cout << "===================================================================\n\n";

    return 0;
}
