#include "core/Vehicle.hpp"
#include <cmath>

namespace dhaka
{

    static constexpr float PI_CONST = 3.14159265358979323846f;

    void Vehicle::setPath(const std::vector<int> &pathNodes, const Graph &graph)
    {
        pathNodeIds = pathNodes;
        currentPathSegmentIndex = 0;
        segmentProgress = 0.0f;

        if (!pathNodeIds.empty())
        {
            currentNodeId = pathNodeIds.front();
            currentPosition = graph.getNode(currentNodeId).position;
        }
    }

    bool Vehicle::stepMovement(float dtSeconds, const Graph &graph)
    {
        if (pathNodeIds.size() < 2 || currentPathSegmentIndex >= static_cast<int>(pathNodeIds.size()) - 1)
        {
            return true; // Path completed or empty
        }

        int u = pathNodeIds[currentPathSegmentIndex];
        int v = pathNodeIds[currentPathSegmentIndex + 1];

        const auto &nodeA = graph.getNode(u);
        const auto &nodeB = graph.getNode(v);

        Vec2 diff = nodeB.position - nodeA.position;
        float segLength = diff.length();

        if (segLength < 1e-4f)
        {
            currentPathSegmentIndex++;
            segmentProgress = 0.0f;
            currentNodeId = v;
            currentPosition = nodeB.position;
            return currentPathSegmentIndex >= static_cast<int>(pathNodeIds.size()) - 1;
        }

        // Determine speed considering road congestion
        int edgeId = graph.getEdgeBetween(u, v);
        float speedMultiplier = 1.0f;
        if (edgeId >= 0)
        {
            const auto &edge = graph.getEdge(edgeId);
            if (edge.isClosed)
            {
                speedMultiplier = 0.0f; // Blocked on closed road
            }
            else
            {
                speedMultiplier = 1.0f / static_cast<float>(edge.congestionFactor);
            }
        }

        float currentSpeed = moveSpeedPixelsPerSec * speedMultiplier;
        float deltaProgress = (currentSpeed * dtSeconds) / segLength;
        segmentProgress += deltaProgress;

        // Track distance
        totalDistanceMeters += (currentSpeed * dtSeconds) * 20.0f; // Scale factor for Dhaka map meters

        // Update heading angle towards next node
        headingAngle = std::atan2(diff.y, diff.x) * (180.0f / PI_CONST);

        if (segmentProgress >= 1.0f)
        {
            segmentProgress = 0.0f;
            currentPathSegmentIndex++;
            currentNodeId = v;
            currentPosition = nodeB.position;

            if (currentPathSegmentIndex >= static_cast<int>(pathNodeIds.size()) - 1)
            {
                return true; // Reached final destination of path
            }
        }
        else
        {
            currentPosition = Vec2::lerp(nodeA.position, nodeB.position, segmentProgress);
        }

        return false; // Still moving along path
    }

    void Vehicle::resetToDepot(const Graph &graph)
    {
        state = VehicleState::IDLE_AT_DEPOT;
        currentLoadKg = 0.0;
        currentNodeId = depotNodeId;
        currentPosition = graph.getNode(depotNodeId).position;
        pathNodeIds.clear();
        currentPathSegmentIndex = 0;
        segmentProgress = 0.0f;
        assignedBinIds.clear();
        currentTargetBinIndex = -1;
        targetLandfillId = -1;
        waitTimerSeconds = 0.0;
    }

} // namespace dhaka
