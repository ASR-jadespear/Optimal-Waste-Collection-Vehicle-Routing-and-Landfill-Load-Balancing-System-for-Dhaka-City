#pragma once

#include "Types.hpp"
#include "Graph.hpp"
#include <vector>
#include <string>

namespace dhaka
{

    class Vehicle
    {
    public:
        int id = -1;
        std::string name;
        Corporation corporation = Corporation::DNCC;
        int depotNodeId = -1;
        int currentNodeId = -1;

        double capacityKg = 6000.0; // e.g., 6 Ton compactor truck
        double currentLoadKg = 0.0;
        VehicleState state = VehicleState::IDLE_AT_DEPOT;

        // Movement & Animation along graph edges
        Vec2 currentPosition;
        float headingAngle = 0.0f;           // in degrees, 0 = right (+X), 90 = down (+Y)
        std::vector<int> pathNodeIds;        // Sequence of node IDs along path
        int currentPathSegmentIndex = 0;     // Segment between pathNodeIds[i] and pathNodeIds[i+1]
        float segmentProgress = 0.0f;        // 0.0f to 1.0f along current edge segment
        float moveSpeedPixelsPerSec = 80.0f; // Visual speed

        // Mission dispatch
        std::vector<int> assignedBinIds;
        int currentTargetBinIndex = -1;
        int targetLandfillId = -1;

        // Timers & operational delays
        double waitTimerSeconds = 0.0;
        double binLoadingDurationSeconds = 4.0;
        double landfillUnloadingDurationSeconds = 6.0;

        // Statistics
        double totalDistanceMeters = 0.0;
        double totalWasteCollectedKg = 0.0;
        int completedTrips = 0;

        Vehicle() = default;

        Vehicle(int id_, const std::string &name_, Corporation corp_, int depotNode,
                const Vec2 &depotPos, double capKg)
            : id(id_), name(name_), corporation(corp_), depotNodeId(depotNode),
              currentNodeId(depotNode), capacityKg(capKg), currentPosition(depotPos) {}

        double getRemainingCapacity() const
        {
            return std::max(0.0, capacityKg - currentLoadKg);
        }

        double getLoadPercentage() const
        {
            return capacityKg > 0.0 ? (currentLoadKg / capacityKg) * 100.0 : 0.0;
        }

        bool isFull() const
        {
            return currentLoadKg >= capacityKg;
        }

        void setPath(const std::vector<int> &pathNodes, const Graph &graph);
        bool stepMovement(float dtSeconds, const Graph &graph);
        void resetToDepot(const Graph &graph);
    };

} // namespace dhaka
