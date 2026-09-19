#include "simulation/SimulationEngine.hpp"
#include <iostream>
#include <sstream>
#include <random>
#include <iomanip>
#include <algorithm>

namespace dhaka
{

    SimulationEngine::SimulationEngine()
    {
        initialize();
    }

    void SimulationEngine::initialize()
    {
        data = DhakaMapBuilder::buildDhakaNetwork();
        simTimeSeconds = 0.0;
        simSpeedMultiplier = 1.0f;
        isPaused = false;
        activeDisruptions.clear();
        nextDisruptionId = 1;
        liveFeedLog.clear();

        runGlobalUrgencyAudit();

        // Initial dispatch of all fleet vehicles
        for (auto &v : data.vehicles)
        {
            dispatchVehicle(v.id);
        }

        updateMetrics();
        lastAlgorithmStatusMessage = "Dhaka Waste Routing Engine initialized. Fleet dispatched.";
        logFeed(lastAlgorithmStatusMessage, 0);
    }

    void SimulationEngine::reset()
    {
        initialize();
    }

    void SimulationEngine::logFeed(const std::string &msg, int stage)
    {
        LiveFeedEntry entry;
        entry.timestamp = simTimeSeconds;
        entry.message = msg;
        entry.relevantStage = stage;
        liveFeedLog.push_back(entry);
        if (liveFeedLog.size() > 50)
        {
            liveFeedLog.erase(liveFeedLog.begin());
        }
    }

    void SimulationEngine::runGlobalUrgencyAudit()
    {
        sortedBinsByUrgency.clear();
        for (auto &bin : data.bins)
        {
            sortedBinsByUrgency.push_back(&bin);
        }

        // Step 1: MergeSort to rank all collection points by urgency (Descending)
        MergeSort::sortByUrgency(sortedBinsByUrgency, true);

        // Step 2: Build PriorityQueue for continuous real-time extraction
        priorityQueue.buildFromPoints(data.bins);
    }

    void SimulationEngine::dispatchVehicle(int vehicleId)
    {
        if (vehicleId < 0 || vehicleId >= static_cast<int>(data.vehicles.size()))
            return;
        auto &v = data.vehicles[vehicleId];

        if (v.state != VehicleState::IDLE_AT_DEPOT)
            return;
        if (v.getRemainingCapacity() <= 50.0)
            return;

        // Filter candidate bins for this vehicle
        std::vector<const CollectionPoint *> candidates;
        for (const auto &bin : data.bins)
        {
            if (bin.isAssigned || bin.currentWasteKg < 30.0)
                continue;

            // Vehicle prefers its own corporation, but can service adjacent points if high urgency
            bool corpMatch = (v.corporation == bin.corporation);
            if (corpMatch || bin.urgencyScore > 0.8)
            {
                candidates.push_back(&bin);
            }
        }

        if (candidates.empty())
            return;

        // Step 2: 0/1 Knapsack (Dynamic Programming) to pick optimal payload subset
        lastKnapsackResult = KnapsackDP::solve(v.getRemainingCapacity(), candidates, 25.0);

        if (lastKnapsackResult.selectedBinIds.empty())
        {
            return;
        }

        // Mark selected bins as assigned so other trucks don't target them simultaneously
        for (int binId : lastKnapsackResult.selectedBinIds)
        {
            data.bins[binId].isAssigned = true;
        }

        // Step 3: Greedy Priority-based Nearest Neighbour to sequence the visit order
        std::vector<int> orderedTour = GreedyTSP::sequenceVisits(
            v.currentNodeId,
            lastKnapsackResult.selectedBinIds,
            data.bins,
            data.graph,
            1.8, 1.0);

        v.assignedBinIds = orderedTour;
        v.currentTargetBinIndex = 0;

        // Step 4: Time-varying A* path to first bin
        int firstBinId = v.assignedBinIds[0];
        int targetNode = data.bins[firstBinId].nodeId;

        PathResult pathRes = Pathfinding::findPathAStar(data.graph, v.currentNodeId, targetNode);
        if (pathRes.found && !pathRes.nodePath.empty())
        {
            v.setPath(pathRes.nodePath, data.graph);
            v.state = VehicleState::EN_ROUTE_TO_BIN;

            std::ostringstream ss;
            ss << v.name << " dispatched via 0/1 Knapsack (DP value: "
               << lastKnapsackResult.totalValue << ", "
               << lastKnapsackResult.itemsSelected << " bins, "
               << static_cast<int>(lastKnapsackResult.totalWeightKg) << " kg payload)";
            lastAlgorithmStatusMessage = ss.str();
            logFeed(lastAlgorithmStatusMessage, 3);
        }
    }

    void SimulationEngine::balanceLandfillsForTruck(int vehicleId)
    {
        if (vehicleId < 0 || vehicleId >= static_cast<int>(data.vehicles.size()))
            return;
        auto &v = data.vehicles[vehicleId];

        // Formulate Max-Flow problem with all trucks currently needing disposal
        std::vector<TruckDemand> demands;
        for (const auto &otherV : data.vehicles)
        {
            if (otherV.id == v.id || otherV.state == VehicleState::EN_ROUTE_TO_LANDFILL)
            {
                int pref = (otherV.corporation == Corporation::DNCC) ? 0 : 1;
                demands.push_back({otherV.id, std::max(500.0, otherV.currentLoadKg), pref, 0.0, 0.0});
            }
        }

        if (demands.empty())
        {
            demands.push_back({v.id, std::max(500.0, v.currentLoadKg),
                               (v.corporation == Corporation::DNCC) ? 0 : 1, 0.0, 0.0});
        }

        // Step 5: Edmonds-Karp Max-Flow to balance Aminbazar vs Matuail intakes
        lastMaxFlowResult = MaxFlow::balanceLandfillLoads(
            demands,
            data.landfills[0], // Aminbazar
            data.landfills[1], // Matuail
            true               // Enforce load balancing
        );

        int assignedLandfillIndex = 0;
        auto it = lastMaxFlowResult.truckToLandfillAssignments.find(v.id);
        if (it != lastMaxFlowResult.truckToLandfillAssignments.end())
        {
            assignedLandfillIndex = it->second;
        }
        else
        {
            assignedLandfillIndex = (v.corporation == Corporation::DNCC) ? 0 : 1;
        }

        v.targetLandfillId = assignedLandfillIndex;
        int landfillNode = data.landfills[assignedLandfillIndex].nodeId;

        // Route to landfill via A*
        PathResult pathRes = Pathfinding::findPathAStar(data.graph, v.currentNodeId, landfillNode);
        if (pathRes.found && !pathRes.nodePath.empty())
        {
            v.setPath(pathRes.nodePath, data.graph);
            v.state = VehicleState::EN_ROUTE_TO_LANDFILL;

            std::ostringstream ss;
            ss << v.name << " routed to " << data.landfills[assignedLandfillIndex].shortName
               << " via Max-Flow Edmonds-Karp (Balance ratio: "
               << static_cast<int>(lastMaxFlowResult.balanceRatio * 100) << "%)";
            lastAlgorithmStatusMessage = ss.str();
            logFeed(lastAlgorithmStatusMessage, 4);
        }
    }

    void SimulationEngine::recalculateVehiclePath(int vehicleId)
    {
        if (vehicleId < 0 || vehicleId >= static_cast<int>(data.vehicles.size()))
            return;
        auto &v = data.vehicles[vehicleId];

        if (v.pathNodeIds.empty() || v.currentPathSegmentIndex >= static_cast<int>(v.pathNodeIds.size()) - 1)
        {
            return;
        }

        int currentOrNextNode = v.pathNodeIds[v.currentPathSegmentIndex + 1];
        int finalTargetNode = v.pathNodeIds.back();

        PathResult newPath = Pathfinding::findPathAStar(data.graph, currentOrNextNode, finalTargetNode);
        if (newPath.found && !newPath.nodePath.empty())
        {
            std::vector<int> stitchedPath;
            stitchedPath.push_back(v.pathNodeIds[v.currentPathSegmentIndex]);
            for (int node : newPath.nodePath)
            {
                stitchedPath.push_back(node);
            }
            v.setPath(stitchedPath, data.graph);
        }
    }

    void SimulationEngine::triggerRoadCongestion(int edgeId, double factor)
    {
        if (edgeId < 0 || edgeId >= data.graph.getEdgeCount())
            return;

        data.graph.setEdgeCongestion(edgeId, factor);

        DisruptionEvent ev;
        ev.id = nextDisruptionId++;
        ev.type = DisruptionType::CONGESTION_SPIKE;
        ev.targetId = edgeId;
        ev.name = data.graph.getEdge(edgeId).roadName;
        ev.description = "Heavy gridlock reported on " + ev.name;
        ev.severity = factor;
        ev.timeTriggeredHours = getSimTimeHours();
        ev.durationMinutes = 45.0;
        ev.isActive = true;
        activeDisruptions.push_back(ev);

        // Dynamic Rerouting: Check all vehicles to reroute if path is affected
        for (auto &v : data.vehicles)
        {
            for (size_t i = v.currentPathSegmentIndex; i + 1 < v.pathNodeIds.size(); ++i)
            {
                int e = data.graph.getEdgeBetween(v.pathNodeIds[i], v.pathNodeIds[i + 1]);
                if (e == edgeId)
                {
                    recalculateVehiclePath(v.id);
                    break;
                }
            }
        }

        lastAlgorithmStatusMessage = "Disruption Alert: Congestion spike on " + ev.name + ". Rerouted affected vehicles via A*.";
        logFeed(lastAlgorithmStatusMessage, 1);
    }

    void SimulationEngine::triggerRoadClosure(int edgeId)
    {
        if (edgeId < 0 || edgeId >= data.graph.getEdgeCount())
            return;

        data.graph.setEdgeClosed(edgeId, true);

        DisruptionEvent ev;
        ev.id = nextDisruptionId++;
        ev.type = DisruptionType::ROAD_CLOSURE;
        ev.targetId = edgeId;
        ev.name = data.graph.getEdge(edgeId).roadName;
        ev.description = "Emergency Road Closure: " + ev.name;
        ev.severity = 999.0;
        ev.timeTriggeredHours = getSimTimeHours();
        ev.durationMinutes = 60.0;
        ev.isActive = true;
        activeDisruptions.push_back(ev);

        for (auto &v : data.vehicles)
        {
            for (size_t i = v.currentPathSegmentIndex; i + 1 < v.pathNodeIds.size(); ++i)
            {
                int e = data.graph.getEdgeBetween(v.pathNodeIds[i], v.pathNodeIds[i + 1]);
                if (e == edgeId)
                {
                    recalculateVehiclePath(v.id);
                    break;
                }
            }
        }

        lastAlgorithmStatusMessage = "Disruption Alert: Road CLOSED (" + ev.name + "). Emergency A* bypass computed.";
        logFeed(lastAlgorithmStatusMessage, 1);
    }

    void SimulationEngine::clearRoadDisruption(int edgeId)
    {
        if (edgeId < 0 || edgeId >= data.graph.getEdgeCount())
            return;
        data.graph.setEdgeCongestion(edgeId, 1.0);
        data.graph.setEdgeClosed(edgeId, false);

        for (auto &ev : activeDisruptions)
        {
            if (ev.targetId == edgeId)
            {
                ev.isActive = false;
            }
        }

        lastAlgorithmStatusMessage = "Traffic normalized on " + data.graph.getEdge(edgeId).roadName;
        logFeed(lastAlgorithmStatusMessage, 0);
    }

    void SimulationEngine::triggerBinOverflow(int binId, double extraWasteKg)
    {
        if (binId < 0 || binId >= static_cast<int>(data.bins.size()))
            return;

        data.bins[binId].addWaste(extraWasteKg, getSimTimeHours());

        // Priority Queue update
        priorityQueue.push(binId, data.bins[binId].urgencyScore, data.bins[binId].currentWasteKg);

        DisruptionEvent ev;
        ev.id = nextDisruptionId++;
        ev.type = DisruptionType::BIN_OVERFLOW_SPIKE;
        ev.targetId = binId;
        ev.name = data.bins[binId].name;
        ev.description = "Sensor alert: Sudden waste surge (+ " + std::to_string(static_cast<int>(extraWasteKg)) + " kg)";
        ev.severity = extraWasteKg;
        ev.timeTriggeredHours = getSimTimeHours();
        ev.durationMinutes = 60.0;
        ev.isActive = true;
        activeDisruptions.push_back(ev);

        // If an idle truck is available, dispatch immediately
        for (auto &v : data.vehicles)
        {
            if (v.state == VehicleState::IDLE_AT_DEPOT)
            {
                dispatchVehicle(v.id);
                break;
            }
        }

        lastAlgorithmStatusMessage = "Overflow Alert at " + data.bins[binId].name + "! Urgency elevated in Priority Queue.";
        logFeed(lastAlgorithmStatusMessage, 2);
    }

    void SimulationEngine::triggerRandomDhakaDisruption()
    {
        static std::mt19937 rng(1337);
        std::uniform_int_distribution<int> typeDist(0, 2);

        int choice = typeDist(rng);
        if (choice == 0)
        {
            // Congestion spike on a random edge
            std::uniform_int_distribution<int> edgeDist(0, data.graph.getEdgeCount() - 1);
            int edgeId = edgeDist(rng);
            triggerRoadCongestion(edgeId, 4.5);
        }
        else if (choice == 1)
        {
            // Road closure on random central edge
            std::uniform_int_distribution<int> edgeDist(0, data.graph.getEdgeCount() - 1);
            int edgeId = edgeDist(rng);
            triggerRoadClosure(edgeId);
        }
        else
        {
            // Sudden overflow at a random bin
            std::uniform_int_distribution<int> binDist(0, static_cast<int>(data.bins.size()) - 1);
            int binId = binDist(rng);
            triggerBinOverflow(binId, 750.0);
        }
    }

    void SimulationEngine::updateBins(double dtHours)
    {
        double currentHours = getSimTimeHours();
        for (auto &bin : data.bins)
        {
            bin.update(dtHours, currentHours);
        }
    }

    void SimulationEngine::updateDisruptions(double dtHours)
    {
        double currentHours = getSimTimeHours();
        for (auto &ev : activeDisruptions)
        {
            if (!ev.isActive)
                continue;
            double elapsedMinutes = (currentHours - ev.timeTriggeredHours) * 60.0;
            if (elapsedMinutes >= ev.durationMinutes)
            {
                ev.isActive = false;
                if (ev.type == DisruptionType::CONGESTION_SPIKE || ev.type == DisruptionType::ROAD_CLOSURE)
                {
                    clearRoadDisruption(ev.targetId);
                }
            }
        }
    }

    void SimulationEngine::updateVehicles(float dtSec)
    {
        for (auto &v : data.vehicles)
        {
            switch (v.state)
            {
            case VehicleState::IDLE_AT_DEPOT:
            {
                dispatchVehicle(v.id);
                break;
            }

            case VehicleState::EN_ROUTE_TO_BIN:
            {
                metrics.totalFleetDistanceKm += (v.moveSpeedPixelsPerSec * dtSec * 20.0) / 1000.0;
                bool reached = v.stepMovement(dtSec, data.graph);
                if (reached)
                {
                    v.state = VehicleState::COLLECTING_WASTE;
                    v.waitTimerSeconds = v.binLoadingDurationSeconds;
                }
                break;
            }

            case VehicleState::COLLECTING_WASTE:
            {
                v.waitTimerSeconds -= dtSec;
                if (v.waitTimerSeconds <= 0.0)
                {
                    // Empty the current target bin
                    if (v.currentTargetBinIndex >= 0 &&
                        v.currentTargetBinIndex < static_cast<int>(v.assignedBinIds.size()))
                    {
                        int binId = v.assignedBinIds[v.currentTargetBinIndex];
                        double collected = data.bins[binId].emptyWaste(getSimTimeHours());
                        v.currentLoadKg += collected;
                        v.totalWasteCollectedKg += collected;
                        metrics.totalWasteCollectedKg += collected;
                    }

                    v.currentTargetBinIndex++;

                    // Determine if there are more bins in tour AND truck has remaining capacity
                    if (v.currentTargetBinIndex < static_cast<int>(v.assignedBinIds.size()) &&
                        v.getLoadPercentage() < 90.0)
                    {
                        int nextBinId = v.assignedBinIds[v.currentTargetBinIndex];
                        int nextNode = data.bins[nextBinId].nodeId;

                        PathResult pathRes = Pathfinding::findPathAStar(data.graph, v.currentNodeId, nextNode);
                        if (pathRes.found && !pathRes.nodePath.empty())
                        {
                            v.setPath(pathRes.nodePath, data.graph);
                            v.state = VehicleState::EN_ROUTE_TO_BIN;
                        }
                        else
                        {
                            // Cannot path to next bin, route to landfill
                            balanceLandfillsForTruck(v.id);
                        }
                    }
                    else
                    {
                        // All assigned bins collected or truck near full: Route to Landfill!
                        balanceLandfillsForTruck(v.id);
                    }
                }
                break;
            }

            case VehicleState::EN_ROUTE_TO_LANDFILL:
            {
                metrics.totalFleetDistanceKm += (v.moveSpeedPixelsPerSec * dtSec * 20.0) / 1000.0;
                bool reached = v.stepMovement(dtSec, data.graph);
                if (reached)
                {
                    v.state = VehicleState::UNLOADING_AT_LANDFILL;
                    v.waitTimerSeconds = v.landfillUnloadingDurationSeconds;
                    if (v.targetLandfillId >= 0 && v.targetLandfillId < static_cast<int>(data.landfills.size()))
                    {
                        data.landfills[v.targetLandfillId].queueCount++;
                    }
                }
                break;
            }

            case VehicleState::UNLOADING_AT_LANDFILL:
            {
                v.waitTimerSeconds -= dtSec;
                if (v.waitTimerSeconds <= 0.0)
                {
                    if (v.targetLandfillId >= 0 && v.targetLandfillId < static_cast<int>(data.landfills.size()))
                    {
                        auto &lf = data.landfills[v.targetLandfillId];
                        lf.dumpWaste(v.currentLoadKg);
                        lf.queueCount = std::max(0, lf.queueCount - 1);
                    }

                    v.currentLoadKg = 0.0;

                    // Route back to home depot
                    PathResult returnPath = Pathfinding::findPathAStar(data.graph, v.currentNodeId, v.depotNodeId);
                    if (returnPath.found && !returnPath.nodePath.empty())
                    {
                        v.setPath(returnPath.nodePath, data.graph);
                        v.state = VehicleState::RETURNING_TO_DEPOT;
                    }
                    else
                    {
                        v.resetToDepot(data.graph);
                    }
                }
                break;
            }

            case VehicleState::RETURNING_TO_DEPOT:
            {
                metrics.totalFleetDistanceKm += (v.moveSpeedPixelsPerSec * dtSec * 20.0) / 1000.0;
                bool reached = v.stepMovement(dtSec, data.graph);
                if (reached)
                {
                    v.state = VehicleState::IDLE_AT_DEPOT;
                    v.completedTrips++;
                    v.assignedBinIds.clear();
                    v.currentTargetBinIndex = -1;
                    v.targetLandfillId = -1;
                }
                break;
            }
            }
        }
    }

    void SimulationEngine::updateMetrics()
    {
        metrics.currentTotalPendingWasteKg = 0.0;
        metrics.currentOverflowingBins = 0;
        double totalUrgency = 0.0;

        for (const auto &bin : data.bins)
        {
            metrics.currentTotalPendingWasteKg += bin.currentWasteKg;
            totalUrgency += bin.urgencyScore;
            if (bin.isOverflowing())
            {
                metrics.currentOverflowingBins++;
                metrics.totalOverflowIncidents++;
            }
        }

        if (!data.bins.empty())
        {
            metrics.averageUrgencyScore = totalUrgency / data.bins.size();
        }

        double totalCapacity = 0.0;
        double totalWaste = 0.0;
        for (const auto &b : data.bins)
        {
            totalCapacity += b.capacityKg;
            totalWaste += b.currentWasteKg;
        }
        if (totalCapacity > 0.0)
        {
            metrics.urgencyCoveragePercent = std::clamp((1.0 - (totalWaste / totalCapacity)) * 100.0, 0.0, 100.0);
        }

        metrics.aminbazarIntakeKg = data.landfills[0].currentIntakeKg;
        metrics.matuailIntakeKg = data.landfills[1].currentIntakeKg;

        if (metrics.aminbazarIntakeKg > 0.0 && metrics.matuailIntakeKg > 0.0)
        {
            double minIntake = std::min(metrics.aminbazarIntakeKg, metrics.matuailIntakeKg);
            double maxIntake = std::max(metrics.aminbazarIntakeKg, metrics.matuailIntakeKg);
            metrics.landfillBalanceRatio = minIntake / maxIntake;
        }
        else
        {
            metrics.landfillBalanceRatio = 1.0;
        }

        int active = 0;
        int trips = 0;
        for (const auto &v : data.vehicles)
        {
            if (v.state != VehicleState::IDLE_AT_DEPOT)
                active++;
            trips += v.completedTrips;
        }
        metrics.activeTruckCount = active;
        metrics.completedTripsCount = trips;
    }

    void SimulationEngine::update(float dtRealSeconds)
    {
        if (isPaused)
            return;

        float dtSec = dtRealSeconds * simSpeedMultiplier;
        simTimeSeconds += dtSec;
        double dtHours = dtSec / 3600.0;

        updateBins(dtHours);
        updateDisruptions(dtHours);
        updateVehicles(dtSec);

        // Periodic urgency audit
        static double lastAuditTime = 0.0;
        if (simTimeSeconds - lastAuditTime >= 30.0)
        {
            lastAuditTime = simTimeSeconds;
            runGlobalUrgencyAudit();
        }

        updateMetrics();
    }

    void SimulationEngine::setSimTimeHours(double hours)
    {
        simTimeSeconds = hours * 3600.0;
        double currentHour = getHourOfDay();

        // Re-evaluate affected vehicle paths with new diurnal edge weights
        for (auto &v : data.vehicles)
        {
            if (v.state == VehicleState::EN_ROUTE_TO_BIN || v.state == VehicleState::EN_ROUTE_TO_LANDFILL)
            {
                recalculateVehiclePath(v.id);
            }
        }

        std::ostringstream ss;
        ss << "Timeline shifted to " << static_cast<int>(currentHour) << ":00 hrs. Time-varying traffic weights updated.";
        logFeed(ss.str(), 0);
    }

    void SimulationEngine::updateRoadEdge(int edgeId, double speedKmh, double congestion, bool isClosed)
    {
        if (edgeId < 0 || edgeId >= data.graph.getEdgeCount())
            return;

        auto &edge = data.graph.getEdge(edgeId);
        edge.baseSpeedKmh = speedKmh;
        edge.congestionFactor = congestion;
        edge.isClosed = isClosed;

        // Also update opposite direction if bidirectional
        int revEdgeId = data.graph.getEdgeBetween(edge.toNode, edge.fromNode);
        if (revEdgeId >= 0 && revEdgeId < data.graph.getEdgeCount())
        {
            auto &revEdge = data.graph.getEdge(revEdgeId);
            revEdge.baseSpeedKmh = speedKmh;
            revEdge.congestionFactor = congestion;
            revEdge.isClosed = isClosed;
        }

        // Selective Downstream Adaptation: only reroute trucks that have this edge in their route!
        int reroutedTrucks = 0;
        for (auto &v : data.vehicles)
        {
            for (size_t i = v.currentPathSegmentIndex; i + 1 < v.pathNodeIds.size(); ++i)
            {
                int e = data.graph.getEdgeBetween(v.pathNodeIds[i], v.pathNodeIds[i + 1]);
                if (e == edgeId || e == revEdgeId)
                {
                    recalculateVehiclePath(v.id);
                    reroutedTrucks++;
                    break;
                }
            }
        }

        std::ostringstream ss;
        ss << "Road updated: " << edge.roadName << " (speed: " << static_cast<int>(speedKmh)
           << " km/h, congestion: " << congestion << "x, " << (isClosed ? "CLOSED" : "OPEN")
           << "). Re-routed " << reroutedTrucks << " active trucks.";
        logFeed(ss.str(), 0);
        lastAlgorithmStatusMessage = ss.str();
    }

    int SimulationEngine::addNewBin(const std::string &name, Vec2 pos, double capacityKg, double initialWasteKg, Corporation corp)
    {
        int nearestNode = data.graph.findNearestNode(pos);
        if (nearestNode < 0)
            nearestNode = 0;

        int newBinId = static_cast<int>(data.bins.size());
        CollectionPoint bin;
        bin.id = newBinId;
        bin.name = name;
        bin.nodeId = nearestNode;
        bin.position = pos;
        bin.capacityKg = capacityKg;
        bin.currentWasteKg = initialWasteKg;
        bin.accumulationRateKgPerHour = 45.0 * wasteGenerationRateMultiplier;
        bin.corporation = corp;
        bin.urgencyScore = (initialWasteKg / capacityKg);

        data.bins.push_back(bin);
        runGlobalUrgencyAudit();

        std::ostringstream ss;
        ss << "New community bin placed: " << name << " at Node #" << nearestNode
           << " (" << static_cast<int>(initialWasteKg) << "/" << static_cast<int>(capacityKg) << " kg).";
        logFeed(ss.str(), 2);
        lastAlgorithmStatusMessage = ss.str();

        return newBinId;
    }

    bool SimulationEngine::removeBin(int binId)
    {
        if (binId < 0 || binId >= static_cast<int>(data.bins.size()))
            return false;

        data.bins[binId].currentWasteKg = 0.0;
        data.bins[binId].isAssigned = false;
        data.bins[binId].urgencyScore = 0.0;

        runGlobalUrgencyAudit();

        std::ostringstream ss;
        ss << "Bin #" << binId << " (" << data.bins[binId].name << ") decommissioned/cleared.";
        logFeed(ss.str(), 2);
        return true;
    }

    void SimulationEngine::generateTraceForStage(int stage)
    {
        activeTrace.clear();

        if (stage == 0) // Road Network Audit
        {
            activeTrace.type = TraceType::ROAD_NETWORK_AUDIT;
            activeTrace.title = "Dhaka Arterial Network Dynamic Weight Audit";

            for (size_t i = 0; i < data.graph.edges.size() && activeTrace.steps.size() < 25; i += 3)
            {
                const auto &edge = data.graph.edges[i];
                AlgorithmStep step;
                step.type = TraceType::ROAD_NETWORK_AUDIT;
                step.activeEdgeId = edge.id;
                step.activeNodeId = edge.fromNode;
                step.targetNodeId = edge.toNode;
                std::ostringstream ss;
                ss << "Edge e" << edge.id << " (" << edge.roadName << "): base speed "
                   << static_cast<int>(edge.baseSpeedKmh) << " km/h, diurnal factor "
                   << std::fixed << std::setprecision(1) << edge.getTimeOfDayMultiplier(getHourOfDay())
                   << "x -> current travel time " << static_cast<int>(edge.getTravelTimeSeconds(getHourOfDay())) << "s.";
                step.narration = ss.str();
                activeTrace.addStep(step);
            }
        }
        else if (stage == 1) // Routing (A*)
        {
            int activeTruckId = 0;
            for (const auto &v : data.vehicles)
            {
                if (!v.pathNodeIds.empty())
                {
                    activeTruckId = v.id;
                    break;
                }
            }
            const auto &v = data.vehicles[activeTruckId];
            int startNode = v.currentNodeId;
            int goalNode = v.pathNodeIds.empty() ? (startNode == 0 ? 12 : 0) : v.pathNodeIds.back();

            Pathfinding::findPathAStar(data.graph, startNode, goalNode, 50.0, getHourOfDay(), &activeTrace);
        }
        else if (stage == 2) // Sequencing (Greedy TSP)
        {
            std::vector<int> candidateBins;
            for (size_t i = 0; i < std::min(size_t(6), data.bins.size()); ++i)
            {
                candidateBins.push_back(static_cast<int>(i));
            }
            GreedyTSP::sequenceVisits(data.vehicles[0].currentNodeId, candidateBins, data.bins, data.graph, 1.8, 1.0, &activeTrace, 0);
        }
        else if (stage == 3) // Load Select (Knapsack DP)
        {
            std::vector<const CollectionPoint *> candidates;
            for (const auto &bin : data.bins)
            {
                if (bin.currentWasteKg >= 30.0)
                {
                    candidates.push_back(&bin);
                }
                if (candidates.size() >= 10)
                    break;
            }
            KnapsackDP::solve(truckDefaultCapacityKg, candidates, 25.0, &activeTrace, 0);
        }
        else if (stage == 4) // Landfill Balance (Max-Flow)
        {
            std::vector<TruckDemand> demands;
            for (const auto &v : data.vehicles)
            {
                demands.push_back({v.id, std::max(1200.0, v.currentLoadKg), (v.corporation == Corporation::DNCC ? 0 : 1), 0.0, 0.0});
            }
            MaxFlow::balanceLandfillLoads(demands, data.landfills[0], data.landfills[1], true, &activeTrace);
        }
    }

} // namespace dhaka
