#pragma once

#include "Types.hpp"
#include <vector>
#include <string>
#include <limits>

namespace dhaka
{

    struct RoadEdge
    {
        int id = -1;
        int fromNode = -1;
        int toNode = -1;
        double lengthMeters = 0.0;
        double baseSpeedKmh = 40.0;
        double congestionFactor = 1.0; // 1.0 = free flow, >1.0 = traffic delay
        bool isClosed = false;
        std::string roadName;

        double getEffectiveSpeedKmh() const
        {
            if (isClosed)
                return 0.0;
            return std::max(2.0, baseSpeedKmh / std::max(1.0, congestionFactor));
        }

        double getTravelTimeSeconds() const
        {
            if (isClosed)
                return std::numeric_limits<double>::infinity();
            double speedMs = (getEffectiveSpeedKmh() * 1000.0) / 3600.0;
            return lengthMeters / speedMs;
        }
    };

    struct RoadNode
    {
        int id = -1;
        std::string name;
        Vec2 position;
        NodeType type = NodeType::INTERSECTION;
        int entityId = -1; // References CollectionPoint, Landfill, or Depot
    };

    class Graph
    {
    public:
        std::vector<RoadNode> nodes;
        std::vector<RoadEdge> edges;
        std::vector<std::vector<int>> adjacency; // node index -> list of outgoing edge indices

        Graph() = default;

        int addNode(const std::string &name, Vec2 pos, NodeType type = NodeType::INTERSECTION, int entityId = -1);

        int addEdge(int from, int to, double lengthMeters, double speedKmh,
                    const std::string &roadName, bool bidirectional = true);

        int getNodeCount() const { return static_cast<int>(nodes.size()); }
        int getEdgeCount() const { return static_cast<int>(edges.size()); }

        const RoadNode &getNode(int id) const { return nodes.at(id); }
        const RoadEdge &getEdge(int id) const { return edges.at(id); }
        RoadEdge &getEdge(int id) { return edges.at(id); }

        const std::vector<int> &getOutgoingEdges(int nodeId) const
        {
            return adjacency.at(nodeId);
        }

        int findNearestNode(const Vec2 &pos) const;
        int findNearestEdge(const Vec2 &pos, float maxDist = 30.0f) const;
        int getEdgeBetween(int u, int v) const;

        void setEdgeCongestion(int edgeId, double factor);
        void setEdgeClosed(int edgeId, bool closed);
        void resetAllCongestion();
    };

} // namespace dhaka
